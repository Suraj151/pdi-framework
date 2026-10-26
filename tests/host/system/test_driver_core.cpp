/****************************** Driver Core Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers the device manager the serial service runs: finding a device on an
interface, binding the driver that matches it, letting go of it when it goes,
and taking in what /etc/devices.conf declares. The interfaces here are fakes
the tests plug chips and usb devices into.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#include <MountedStack.h>
#include <ShellHarness.h>
#include <pditest.h>

#include <service_provider/transport/DeviceManager.h>
#include <service_provider/transport/SerialServiceProvider.h>
#include <service_provider/network/SyslogServiceProvider.h>
#include <interface/pdi/modules/serial/iI2cSerialInterface.h>
#include <interface/pdi/modules/serial/iUsbHostSerialInterface.h>
#include <interface/pdi/modules/serial/iI2sSerialInterface.h>
#include <interface/pdi/impl/drivers/BuiltInDrivers.h>
#ifdef ENABLE_I2S_BUS
#include <interface/pdi/impl/drivers/Inmp441Driver.h>
#endif
#ifdef ENABLE_I2C_BUS
#include <interface/pdi/impl/drivers/At24Driver.h>
#endif
#ifdef ENABLE_SPI_BUS
#include <interface/pdi/modules/serial/iSpiSerialInterface.h>
#include <interface/pdi/impl/drivers/MmcSpiDriver.h>
#include <devices/posix/SerialInterface.h>
#endif
#ifdef ENABLE_FAT_FS
#include <interface/pdi/impl/modules/storage/FatFsInterfaceImpl.h>
#include <external/fatfs/diskio.h>
#endif

#include <functional>
#include <map>
#include <set>
#include <vector>

#ifdef ENABLE_DRIVER_CORE

namespace
{

const uint8_t TESTCHIP_ID = 0x42;
const uint8_t TESTCHIP_ADDRESS = 0x50;
const uint8_t USB_CLASS_HID = 0x03;

int g_probes = 0;
int g_removes = 0;
bool g_testchip_busy = false;
int g_added_events = 0;
int g_removed_events = 0;

/**
 * An I2C bus with chips plugged in by the test, each a run of register bytes
 * starting at register 0.
 */
class FakeI2c : public iI2cSerialInterface
{
public:
    std::map<uint32_t, std::vector<uint8_t>> m_chips;
    std::set<uint32_t> m_probed;
    std::vector<uint8_t> m_defaults;
    pdi_err_t m_configure_result = PDI_OK;
    uint32_t m_probe_ms = 0;

    uint8_t defaultPins(uint8_t *out, uint8_t max) const override
    {
        uint8_t n = 0;
        for (; n < m_defaults.size() && n < max; n++)
        {
            out[n] = m_defaults[n];
        }
        return n;
    }

    pdi_err_t configure(int16_t sda, int16_t scl, uint32_t clock) override
    {
        if (sda >= 0 && PDI_OK != m_configure_result)
        {
            return m_configure_result;
        }
        return iI2cSerialInterface::configure(sda, scl, clock);
    }

    bool probe(uint32_t target) override
    {
        m_probed.insert(target);
        __i_dvc_ctrl.advanceVirtualClock((uint64_t)m_probe_ms * 1000ULL);
        return m_chips.count(target) > 0;
    }

    int32_t transfer(const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen) override
    {
        if (0 == m_chips.count(m_selected))
        {
            return PDI_ERR_NOT_FOUND;
        }
        std::vector<uint8_t> &regs = m_chips[m_selected];
        uint8_t reg = (nullptr != tx && txlen > 0) ? tx[0] : 0;
        for (uint32_t i = 0; i < rxlen && nullptr != rx; i++)
        {
            rx[i] = (reg + i < regs.size()) ? regs[reg + i] : 0xFF;
        }
        return (int32_t)(rxlen > 0 ? rxlen : txlen);
    }
};

/**
 * A usb host port with devices plugged in by the test.
 */
class FakeUsb : public iUsbHostSerialInterface
{
public:
    std::vector<usb_device_info_t> m_attached;

    uint8_t deviceCount() override { return (uint8_t)m_attached.size(); }

    bool deviceAt(uint8_t index, usb_device_info_t &out) override
    {
        if (index >= m_attached.size())
        {
            return false;
        }
        out = m_attached[index];
        return true;
    }

    void plug(uint8_t address, uint16_t vid, uint16_t pid, uint8_t cls)
    {
        usb_device_info_t info;
        info.m_address = address;
        info.m_vid = vid;
        info.m_pid = pid;
        info.m_class = cls;
        m_attached.push_back(info);
    }
};

/**
 * An I2S bus whose microphone the test switches between changing samples and
 * a flat line, the way an unconnected data pin reads.
 */
class FakeI2s : public iI2sSerialInterface
{
public:
    bool m_noise = true;
    bool m_configured = false;
    int m_flat_reads = 0;
    int m_configures = 0;
    int m_releases = 0;
    uint8_t m_seed = 1;
    serial_i2s_config_t m_config;

    pdi_err_t m_configure_result = PDI_OK;
    int32_t m_read_result = 0;
    uint32_t m_read_cap = 0;
    std::vector<uint8_t> m_defaults;

    uint8_t defaultPins(uint8_t *out, uint8_t max) const override
    {
        uint8_t n = 0;
        for (; n < m_defaults.size() && n < max; n++)
        {
            out[n] = m_defaults[n];
        }
        return n;
    }

    pdi_err_t configure(const serial_i2s_config_t &config) override
    {
        m_config = config;
        m_configures++;
        if (PDI_OK != m_configure_result)
        {
            return m_configure_result;
        }
        m_configured = true;
        return PDI_OK;
    }

    void release() override
    {
        if (m_configured)
        {
            m_releases++;
        }
        m_configured = false;
    }

    int32_t write(const uint8_t *buf, uint32_t size) override { return (int32_t)size; }

    int32_t read(uint8_t *buf, uint32_t size) override
    {
        if (!m_configured || nullptr == buf)
        {
            return 0;
        }
        if (0 > m_read_result)
        {
            return m_read_result;
        }
        bool flat = !m_noise || m_flat_reads > 0;
        if (m_flat_reads > 0)
        {
            m_flat_reads--;
        }
        if (m_read_cap > 0 && size > m_read_cap)
        {
            size = m_read_cap;
        }
        for (uint32_t i = 0; i < size; i++)
        {
            buf[i] = flat ? 0 : (uint8_t)(i * 37 + m_seed++);
        }
        return (int32_t)size;
    }

    uint32_t timeout() const { return m_timeout; }
};

#ifdef ENABLE_SPI_BUS
/**
 * An SPI bus with cards plugged in behind chip selects by the test. It keeps
 * the clock and chip select of every transaction and counts the bytes clocked
 * with no chip select low.
 */
class FakeSpi : public iSpiSerialInterface
{
public:
    std::map<int16_t, SdCardSim *> m_cards;
    std::vector<std::pair<int16_t, uint32_t>> m_begins;
    std::vector<uint8_t> m_defaults;
    pdi_err_t m_configure_result = PDI_OK;
    int m_idle_bytes = 0;
    bool m_open = false;
    int m_fail_begin = 0;
    pdi_err_t m_begin_result = PDI_ERR_BUSY;

    uint8_t defaultPins(uint8_t *out, uint8_t max) const override
    {
        uint8_t n = 0;
        for (; n < m_defaults.size() && n < max; n++)
        {
            out[n] = m_defaults[n];
        }
        return n;
    }

    pdi_err_t configure(int16_t sck, int16_t miso, int16_t mosi, uint32_t clock) override
    {
        if (sck >= 0 && PDI_OK != m_configure_result)
        {
            return m_configure_result;
        }
        return iSpiSerialInterface::configure(sck, miso, mosi, clock);
    }

    pdi_err_t begin(int16_t cs, uint32_t clock, uint8_t mode) override
    {
        m_begins.push_back(std::make_pair(cs, clockFor(clock)));
        if (m_fail_begin > 0 && (int)m_begins.size() == m_fail_begin)
        {
            return m_begin_result;
        }
        m_cs = cs;
        m_open = true;
        SdCardSim *card = cardAt(cs);
        if (nullptr != card)
        {
            card->select(true);
        }
        return PDI_OK;
    }

    void end() override
    {
        SdCardSim *card = cardAt(m_cs);
        if (nullptr != card)
        {
            card->select(false);
        }
        m_cs = -1;
        m_open = false;
    }

    int32_t exchange(const uint8_t *tx, uint8_t *rx, uint32_t len) override
    {
        SdCardSim *card = cardAt(m_cs);
        for (uint32_t i = 0; i < len; i++)
        {
            uint8_t in = (nullptr != tx) ? tx[i] : 0xFF;
            uint8_t out = (nullptr != card) ? card->exchange(in) : 0xFF;
            if (m_cs < 0)
            {
                m_idle_bytes++;
            }
            if (nullptr != rx)
            {
                rx[i] = out;
            }
        }
        return (int32_t)len;
    }

    SdCardSim *cardAt(int16_t cs)
    {
        return (cs >= 0 && m_cards.count(cs) > 0) ? m_cards[cs] : nullptr;
    }

private:
    int16_t m_cs = -1;
};
#endif

/**
 * Takes the device only when its id register reads the id it expects.
 */
class TestChip : public iDriverInterface
{
public:
    pdi_err_t probe(driver_device_t &dev, iSerialInterface *bus) override
    {
        g_probes++;
        if (nullptr == bus || SERIAL_IFACE_I2C != serialFamily(bus->type()))
        {
            return PDI_ERR_NOT_SUPPORTED;
        }
        iI2cSerialInterface *i2c = static_cast<iI2cSerialInterface *>(bus);
        uint8_t id = 0;
        i2c->select(dev.m_target);
        if (1 != i2c->readRegister(0, &id, 1) || TESTCHIP_ID != id)
        {
            return PDI_ERR_NOT_FOUND;
        }
        return PDI_OK;
    }

    void remove(driver_device_t &dev) override { g_removes++; }

    bool busy() const override { return g_testchip_busy; }
};

/**
 * Takes any usb device of the hid class.
 */
class TestHid : public iDriverInterface
{
public:
    pdi_err_t probe(driver_device_t &dev, iSerialInterface *bus) override
    {
        g_probes++;
        return PDI_OK;
    }

    void remove(driver_device_t &dev) override { g_removes++; }
};

const uint8_t s_testchip_addresses[] = {TESTCHIP_ADDRESS};

iDriverInterface *createTestChip() { return pdiutil::safe_new<TestChip>(); }
iDriverInterface *createTestHid() { return pdiutil::safe_new<TestHid>(); }
bool matchHid(const driver_device_t &dev) { return USB_CLASS_HID == dev.m_class; }

/**
 * The test drivers, registered once for the whole file.
 */
void registerTestDrivers()
{
    static bool registered = false;
    if (registered)
    {
        return;
    }
    registered = true;
#if defined(ENABLE_I2S_BUS) || defined(ENABLE_I2C_BUS) || defined(ENABLE_SPI_BUS)
    registerBuiltInDrivers();
#endif
    DeviceManager::registerDriver(driver_prop_t("testchip", SERIAL_IFACE_I2C, s_testchip_addresses,
                                                sizeof(s_testchip_addresses), nullptr, createTestChip));
    DeviceManager::registerDriver(driver_prop_t("testhid", SERIAL_IFACE_USB_HOST, nullptr, 0,
                                                matchHid, createTestHid));
    __utl_event.add_event_listener(EVENT_DEVICE_ADDED, [](void *) { g_added_events++; });
    __utl_event.add_event_listener(EVENT_DEVICE_REMOVED, [](void *) { g_removed_events++; });
}

void writeDevicesConf(const char *body)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    fs->beginPrivileged();
    if (!fs->isFileExist("/etc"))
    {
        fs->createDirectory("/etc");
    }
    if (fs->isFileExist(DEVICES_CONF_PATH))
    {
        fs->deleteFile(DEVICES_CONF_PATH);
    }
    if (nullptr != body)
    {
        fs->createFile(DEVICES_CONF_PATH, body, (int64_t)strlen(body));
    }
    fs->endPrivileged();
}

/**
 * Puts fake interfaces where the port's would be, and the port's back after.
 */
struct Bench
{
    FakeI2c m_i2c;
    FakeUsb m_usb;
    FakeI2s m_i2s;
#ifdef ENABLE_SPI_BUS
    FakeSpi m_spi;
#endif
    iSerialInterface *m_saved_i2c;
    iSerialInterface *m_saved_usb;
    iSerialInterface *m_saved_i2s;
    iSerialInterface *m_saved_spi;

    Bench()
    {
        registerTestDrivers();
        writeDevicesConf(nullptr);
        g_probes = 0;
        g_removes = 0;
        g_added_events = 0;
        g_removed_events = 0;
        m_saved_i2c = iSerialInterface::instances[SERIAL_IFACE_I2C];
        m_saved_usb = iSerialInterface::instances[SERIAL_IFACE_USB_HOST];
        m_saved_i2s = iSerialInterface::instances[SERIAL_IFACE_I2S];
        m_saved_spi = iSerialInterface::instances[SERIAL_IFACE_SPI];
        iSerialInterface::instances[SERIAL_IFACE_I2C] = &m_i2c;
        iSerialInterface::instances[SERIAL_IFACE_USB_HOST] = &m_usb;
        iSerialInterface::instances[SERIAL_IFACE_I2S] = &m_i2s;
#ifdef ENABLE_SPI_BUS
        iSerialInterface::instances[SERIAL_IFACE_SPI] = &m_spi;
#endif
    }

    ~Bench()
    {
        iSerialInterface::instances[SERIAL_IFACE_I2C] = m_saved_i2c;
        iSerialInterface::instances[SERIAL_IFACE_USB_HOST] = m_saved_usb;
        iSerialInterface::instances[SERIAL_IFACE_I2S] = m_saved_i2s;
        iSerialInterface::instances[SERIAL_IFACE_SPI] = m_saved_spi;
        writeDevicesConf(nullptr);
    }

    void plugChip(uint32_t address, uint8_t id)
    {
        m_i2c.m_chips[address] = std::vector<uint8_t>{id, 0x01, 0x02};
    }
};

std::string idOf(const driver_device_t &dev)
{
    pdiutil::string out;
    DeviceManager::deviceId(dev, out);
    return std::string(out.c_str());
}

/**
 * The manager the serial service runs, started once for the whole file.
 */
DeviceManager *serviceManager()
{
    static bool started = false;
    if (!started)
    {
        started = true;
        __serial_service.initService();
    }
    return __serial_service.devices();
}

/**
 * The bench, with the serial service's manager brought up to date with it
 * and left clean of its fakes once the test is done.
 */
struct ServiceBench : Bench
{
    DeviceManager *m_mgr;

    ServiceBench() : m_mgr(serviceManager())
    {
        if (nullptr != m_mgr)
        {
            m_mgr->trigger();
        }
    }

    ~ServiceBench()
    {
        m_i2c.m_chips.clear();
        m_usb.m_attached.clear();
        writeDevicesConf(nullptr);
        if (nullptr != m_mgr)
        {
            m_mgr->trigger();
        }
    }

    void scan()
    {
        if (nullptr != m_mgr)
        {
            m_mgr->scan();
        }
    }

    const driver_device_t *find(serial_iface_t bus, uint32_t target)
    {
        return (nullptr == m_mgr) ? nullptr : m_mgr->find(bus, target);
    }
};

} // namespace

TEST(driver_core, a_chip_on_the_bus_is_found_and_bound)
{
    Bench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    DeviceManager mgr;

    mgr.scan();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_STREQ(dev->m_driver.c_str(), "testchip");
    ASSERT_EQ(g_probes, 1);
    ASSERT_EQ(g_added_events, 1);
}

TEST(driver_core, a_device_that_stays_is_not_probed_again)
{
    Bench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    DeviceManager mgr;

    mgr.scan();
    mgr.scan();
    mgr.scan();

    ASSERT_EQ(g_probes, 1);
    ASSERT_EQ(g_added_events, 1);
    ASSERT_EQ((int)mgr.count(), 1);
}

TEST(driver_core, a_chip_that_goes_away_is_removed)
{
    Bench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    DeviceManager mgr;
    mgr.scan();

    bench.m_i2c.m_chips.clear();
    mgr.scan();

    ASSERT_NULL(mgr.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS));
    ASSERT_EQ(g_removes, 1);
    ASSERT_EQ(g_removed_events, 1);
    ASSERT_EQ((int)mgr.count(), 0);
}

TEST(driver_core, a_chip_that_comes_back_is_bound_again)
{
    Bench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    DeviceManager mgr;
    mgr.scan();
    bench.m_i2c.m_chips.clear();
    mgr.scan();

    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    mgr.scan();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_EQ(g_probes, 2);
}

TEST(driver_core, a_chip_that_is_not_the_one_the_driver_drives_is_left_unbound)
{
    Bench bench;
    bench.plugChip(TESTCHIP_ADDRESS, 0x99);
    DeviceManager mgr;

    mgr.scan();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_PRESENT);
    ASSERT_NULL(dev->m_instance);
    ASSERT_EQ(g_removes, 0);
}

TEST(driver_core, an_address_no_driver_lists_is_never_probed)
{
    Bench bench;
    bench.plugChip(0x33, TESTCHIP_ID);
    DeviceManager mgr;

    mgr.scan();

    ASSERT_EQ((int)bench.m_i2c.m_probed.count(0x33), 0);
    ASSERT_EQ((int)bench.m_i2c.m_probed.count(TESTCHIP_ADDRESS), 1);
    ASSERT_EQ((int)mgr.count(), 0);
}

TEST(driver_core, a_usb_device_is_bound_by_its_class)
{
    Bench bench;
    bench.m_usb.plug(2, 0x046d, 0xc31c, USB_CLASS_HID);
    DeviceManager mgr;

    mgr.scan();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_USB_HOST, 2);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_STREQ(dev->m_driver.c_str(), "testhid");
    ASSERT_EQ((int)dev->m_vid, 0x046d);
    ASSERT_EQ((int)dev->m_pid, 0xc31c);
}

TEST(driver_core, a_usb_device_no_driver_matches_is_known_but_unbound)
{
    Bench bench;
    bench.m_usb.plug(3, 0x0781, 0x5567, 0x08);
    DeviceManager mgr;

    mgr.scan();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_USB_HOST, 3);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_PRESENT);
    ASSERT_EQ(g_probes, 0);
    ASSERT_EQ(g_added_events, 1);
}

TEST(driver_core, a_usb_device_unplugged_is_removed)
{
    Bench bench;
    bench.m_usb.plug(2, 0x046d, 0xc31c, USB_CLASS_HID);
    DeviceManager mgr;
    mgr.scan();

    bench.m_usb.m_attached.clear();
    mgr.scan();

    ASSERT_NULL(mgr.find(SERIAL_IFACE_USB_HOST, 2));
    ASSERT_EQ(g_removes, 1);
    ASSERT_EQ(g_removed_events, 1);
}

TEST(driver_core, a_declared_device_is_bound_by_the_driver_it_names)
{
    Bench bench;
    bench.plugChip(0x60, TESTCHIP_ID);
    writeDevicesConf("i2c0 0x60 testchip\n");
    DeviceManager mgr;

    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x60);
    ASSERT_NOT_NULL(dev);
    ASSERT_TRUE(dev->m_declared);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_STREQ(dev->m_driver.c_str(), "testchip");
}

TEST(driver_core, a_declared_device_that_is_absent_stays_known)
{
    Bench bench;
    writeDevicesConf("i2c0 0x61 testchip\n");
    DeviceManager mgr;

    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x61);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_ABSENT);
    ASSERT_EQ(g_probes, 0);
}

TEST(driver_core, a_declared_device_that_goes_away_stays_known_as_absent)
{
    Bench bench;
    bench.plugChip(0x60, TESTCHIP_ID);
    writeDevicesConf("i2c0 0x60 testchip\n");
    DeviceManager mgr;
    mgr.trigger();

    bench.m_i2c.m_chips.clear();
    mgr.scan();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x60);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_ABSENT);
    ASSERT_EQ(g_removes, 1);
}

TEST(driver_core, a_last_declaration_line_with_no_newline_is_still_read)
{
    Bench bench;
    bench.plugChip(0x60, TESTCHIP_ID);
    writeDevicesConf("i2c0 0x60 testchip chip=last");
    DeviceManager mgr;

    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x60);
    ASSERT_NOT_NULL(dev);
    ASSERT_TRUE(dev->m_declared);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_STREQ(dev->m_params.c_str(), "chip=last");
}

TEST(driver_core, a_last_bus_line_or_bad_line_with_no_newline_is_still_read)
{
    Bench bench;
    bench.m_i2c.m_defaults = {4, 5};
    writeDevicesConf("i2c0 bus i2c sda=12 scl=13");
    DeviceManager mgr;
    mgr.trigger();
    ASSERT_EQ((int)mgr.busResult(SERIAL_IFACE_I2C), (int)PDI_OK);
    ASSERT_EQ((int)bench.m_i2c.sda(), 12);

    writeDevicesConf("i2c0 0x60 testchip\nnot a line");
    mgr.trigger();
    ASSERT_EQ((int)mgr.count(), 1);
    ASSERT_NOT_NULL(mgr.find(SERIAL_IFACE_I2C, 0x60));
}

TEST(driver_core, a_found_device_declared_later_is_let_go_and_bound_again_as_declared)
{
    Bench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    DeviceManager mgr;
    mgr.scan();
    const driver_device_t *found = mgr.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS);
    ASSERT_NOT_NULL(found);
    ASSERT_FALSE(found->m_declared);
    ASSERT_EQ((int)found->m_state, (int)DEVICE_STATE_BOUND);

    writeDevicesConf("i2c0 0x50 testchip mode=declared\n");
    mgr.scan();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS);
    ASSERT_TRUE(dev->m_declared);
    ASSERT_STREQ(dev->m_params.c_str(), "mode=declared");
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_EQ(g_removes, 1);
    ASSERT_EQ((int)mgr.count(), 1);
}

TEST(driver_core, a_manager_going_while_a_driver_is_busy_waits_then_lets_it_go)
{
    Bench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    uint32_t began = 0;
    {
        DeviceManager mgr;
        mgr.scan();
        ASSERT_EQ((int)mgr.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS)->m_state, (int)DEVICE_STATE_BOUND);
        g_testchip_busy = true;
        began = __i_dvc_ctrl.millis_now();
    }
    uint32_t waited = __i_dvc_ctrl.millis_now() - began;
    g_testchip_busy = false;

    ASSERT_TRUE(waited >= DEVICE_BUSY_WAIT_MS);
    ASSERT_EQ(g_removes, 1);
}

namespace
{

/**
 * The bench on a clock that only moves when the bus's probe moves it, so a
 * slow probe costs no real time.
 */
struct SlowBusBench : Bench
{
    SlowBusBench() { __i_dvc_ctrl.useVirtualClock(true); }
    ~SlowBusBench() { __i_dvc_ctrl.useVirtualClock(false); }
};

} // namespace

TEST(driver_core, an_absent_device_whose_probe_is_slow_is_left_out_of_the_scan)
{
    SlowBusBench bench;
    writeDevicesConf("i2c0 0x60 testchip\n");
    bench.m_i2c.m_probe_ms = DEVICE_SCAN_PROBE_MAX_MS + 1;
    DeviceManager mgr;
    mgr.trigger();

    bench.m_i2c.m_probed.clear();
    mgr.scan();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x60);
    ASSERT_NOT_NULL(dev);
    ASSERT_TRUE(dev->m_unscanned);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_ABSENT);
    ASSERT_EQ((int)bench.m_i2c.m_probed.count(0x60), 0);
}

TEST(driver_core, a_probe_that_takes_the_limit_exactly_stays_in_the_scan)
{
    SlowBusBench bench;
    writeDevicesConf("i2c0 0x60 testchip\n");
    bench.m_i2c.m_probe_ms = DEVICE_SCAN_PROBE_MAX_MS;
    DeviceManager mgr;
    mgr.trigger();

    bench.m_i2c.m_probed.clear();
    mgr.scan();

    ASSERT_FALSE(mgr.find(SERIAL_IFACE_I2C, 0x60)->m_unscanned);
    ASSERT_EQ((int)bench.m_i2c.m_probed.count(0x60), 1);
}

TEST(driver_core, a_device_left_out_comes_back_in_on_a_trigger_once_its_probe_is_fast)
{
    SlowBusBench bench;
    writeDevicesConf("i2c0 0x60 testchip\n");
    bench.m_i2c.m_probe_ms = DEVICE_SCAN_PROBE_MAX_MS + 1;
    DeviceManager mgr;
    mgr.trigger();

    bench.m_i2c.m_probe_ms = 0;
    bench.plugChip(0x60, TESTCHIP_ID);
    mgr.scan();
    ASSERT_EQ((int)mgr.find(SERIAL_IFACE_I2C, 0x60)->m_state, (int)DEVICE_STATE_ABSENT);

    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x60);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_FALSE(dev->m_unscanned);
    bench.m_i2c.m_probed.clear();
    mgr.scan();
    ASSERT_EQ((int)bench.m_i2c.m_probed.count(0x60), 1);
}

TEST(driver_core, a_trigger_that_finds_the_probe_still_slow_leaves_the_device_out)
{
    SlowBusBench bench;
    writeDevicesConf("i2c0 0x60 testchip\n");
    bench.m_i2c.m_probe_ms = DEVICE_SCAN_PROBE_MAX_MS + 1;
    DeviceManager mgr;
    mgr.trigger();

    mgr.trigger();
    bench.m_i2c.m_probed.clear();
    mgr.scan();

    ASSERT_TRUE(mgr.find(SERIAL_IFACE_I2C, 0x60)->m_unscanned);
    ASSERT_EQ((int)bench.m_i2c.m_probed.count(0x60), 0);
}

TEST(driver_core, a_bound_device_whose_probe_turns_slow_stays_bound_and_is_left_out)
{
    SlowBusBench bench;
    bench.plugChip(0x60, TESTCHIP_ID);
    writeDevicesConf("i2c0 0x60 testchip\n");
    DeviceManager mgr;
    mgr.trigger();

    bench.m_i2c.m_probe_ms = DEVICE_SCAN_PROBE_MAX_MS + 1;
    mgr.scan();
    bench.m_i2c.m_chips.clear();
    bench.m_i2c.m_probed.clear();
    mgr.scan();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x60);
    ASSERT_TRUE(dev->m_unscanned);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_EQ(g_removes, 0);
    ASSERT_EQ((int)bench.m_i2c.m_probed.count(0x60), 0);
}

TEST(driver_core, a_slow_probe_leaves_the_bus_working_for_the_bound_driver)
{
    SlowBusBench bench;
    bench.plugChip(0x60, TESTCHIP_ID);
    writeDevicesConf("i2c0 0x60 testchip\n");
    DeviceManager mgr;
    mgr.trigger();

    bench.m_i2c.m_probe_ms = DEVICE_SCAN_PROBE_MAX_MS + 1;
    mgr.scan();
    uint8_t reg = 0;
    uint8_t got = 0;

    ASSERT_EQ((int)bench.m_i2c.transferTo(0x60, &reg, 1, &got, 1), 1);
    ASSERT_EQ((int)got, (int)TESTCHIP_ID);
}

TEST(driver_core, a_changed_declaration_file_is_read_again_on_the_next_scan)
{
    Bench bench;
    writeDevicesConf("i2c0 0x61 testchip\n");
    DeviceManager mgr;
    mgr.scan();
    ASSERT_EQ((int)mgr.count(), 1);

    writeDevicesConf("i2c0 0x61 testchip\ni2c0 0x62 testchip\n");
    mgr.scan();

    ASSERT_EQ((int)mgr.count(), 2);
    ASSERT_NOT_NULL(mgr.find(SERIAL_IFACE_I2C, 0x62));
}

TEST(driver_core, a_declaration_taken_out_of_the_file_is_forgotten)
{
    Bench bench;
    writeDevicesConf("i2c0 0x61 testchip\ni2c0 0x62 testchip\n");
    DeviceManager mgr;
    mgr.scan();

    writeDevicesConf("i2c0 0x61 testchip\n");
    mgr.scan();

    ASSERT_EQ((int)mgr.count(), 1);
    ASSERT_NULL(mgr.find(SERIAL_IFACE_I2C, 0x62));
}

TEST(driver_core, a_bad_declaration_is_skipped_and_the_rest_are_taken)
{
    Bench bench;
    writeDevicesConf("i2c0 0xZZ testchip\n# a comment\n\ni2c0 0x61 testchip\n");
    DeviceManager mgr;

    mgr.trigger();

    ASSERT_EQ((int)mgr.count(), 1);
    ASSERT_NOT_NULL(mgr.find(SERIAL_IFACE_I2C, 0x61));
}

TEST(driver_core, a_manager_that_goes_lets_go_of_every_bound_device)
{
    Bench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    bench.m_usb.plug(2, 0x046d, 0xc31c, USB_CLASS_HID);
    {
        DeviceManager mgr;
        mgr.scan();
        ASSERT_EQ(g_probes, 2);
    }

    ASSERT_EQ(g_removes, 2);
}

TEST(driver_core, an_interface_the_port_does_not_have_is_passed_over)
{
    Bench bench;
    iSerialInterface::instances[SERIAL_IFACE_I2C] = nullptr;
    iSerialInterface::instances[SERIAL_IFACE_USB_HOST] = nullptr;
    DeviceManager mgr;

    mgr.scan();

    ASSERT_EQ((int)mgr.count(), 0);
}

TEST(driver_core, a_declaration_line_reads_into_a_device)
{
    driver_device_t dev;

    ASSERT_TRUE(DeviceManager::parseLine("i2c0 0x76 bme280", 16, dev));
    ASSERT_EQ((int)dev.m_bus, (int)SERIAL_IFACE_I2C);
    ASSERT_EQ((int)dev.m_target, 0x76);
    ASSERT_STREQ(dev.m_driver.c_str(), "bme280");
    ASSERT_TRUE(dev.m_declared);

    ASSERT_TRUE(DeviceManager::parseLine("spi1\tcs=5\tspiflash", 18, dev));
    ASSERT_EQ((int)dev.m_bus, (int)SERIAL_IFACE_SPI1);
    ASSERT_EQ((int)dev.m_target, 5);

    ASSERT_TRUE(DeviceManager::parseLine("i2s0 rx mic", 11, dev));
    ASSERT_EQ((int)dev.m_bus, (int)SERIAL_IFACE_I2S);
    ASSERT_EQ((int)dev.m_target, 0);

    ASSERT_TRUE(DeviceManager::parseLine("i2s0 tx speaker", 15, dev));
    ASSERT_EQ((int)dev.m_target, 1);

    ASSERT_TRUE(DeviceManager::parseLine("usb0 3 keyboard", 15, dev));
    ASSERT_EQ((int)dev.m_bus, (int)SERIAL_IFACE_USB_HOST);
    ASSERT_EQ((int)dev.m_target, 3);

    ASSERT_TRUE(DeviceManager::parseLine("  i2c1 96 bme280  ", 18, dev));
    ASSERT_EQ((int)dev.m_bus, (int)SERIAL_IFACE_I2C1);
    ASSERT_EQ((int)dev.m_target, 96);
}

TEST(driver_core, a_line_that_is_not_a_declaration_is_refused)
{
    driver_device_t dev;
    const char *refused[] = {
        "",
        "   ",
        "# i2c0 0x76 bme280",
        "i2c9 0x76 bme280",
        "eth0 0x76 bme280",
        "i2c0 0xZZ bme280",
        "i2c0 0x bme280",
        "i2c0 0x76",
        "i2c0",
        "i2c0 0x76 bme280 extra",
        "i2c0 0x1ffffffff bme280",
        "spi0 cs= spiflash",
        "i2s0 sideways mic",
    };

    for (size_t i = 0; i < sizeof(refused) / sizeof(refused[0]); i++)
    {
        ASSERT_FALSE(DeviceManager::parseLine(refused[i], (uint16_t)strlen(refused[i]), dev));
    }
}

TEST(driver_core, a_declaration_carries_settings_for_its_driver)
{
    driver_device_t dev;
    const char *line = "i2s0 rx inmp441 sck=32 ws=33 sd=34 rate=16000";

    ASSERT_TRUE(DeviceManager::parseLine(line, (uint16_t)strlen(line), dev));
    ASSERT_STREQ(dev.m_driver.c_str(), "inmp441");

    uint32_t value = 0;
    ASSERT_TRUE(DeviceManager::param(dev, "ws", value));
    ASSERT_EQ((int)value, 33);
    ASSERT_TRUE(DeviceManager::param(dev, "rate", value));
    ASSERT_EQ((int)value, 16000);
    ASSERT_FALSE(DeviceManager::param(dev, "mclk", value));
    ASSERT_FALSE(DeviceManager::param(dev, "s", value));
}

TEST(driver_core, a_setting_that_is_not_key_equals_value_is_refused)
{
    driver_device_t dev;
    const char *refused[] = {
        "i2s0 rx inmp441 sck32",
        "i2s0 rx inmp441 =3",
        "i2s0 rx inmp441 sck=",
        "i2s0 rx inmp441 sck=3 ws",
    };

    for (size_t i = 0; i < sizeof(refused) / sizeof(refused[0]); i++)
    {
        ASSERT_FALSE(DeviceManager::parseLine(refused[i], (uint16_t)strlen(refused[i]), dev));
    }
}

TEST(driver_core, a_changed_setting_is_a_changed_declaration)
{
    Bench bench;
    bench.plugChip(0x60, TESTCHIP_ID);
    writeDevicesConf("i2c0 0x60 testchip speed=100000\n");
    DeviceManager mgr;
    mgr.trigger();
    ASSERT_EQ(g_probes, 1);

    writeDevicesConf("i2c0 0x60 testchip speed=400000\n");
    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x60);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_EQ(g_probes, 2);
    ASSERT_EQ(g_removes, 1);
    uint32_t speed = 0;
    ASSERT_TRUE(DeviceManager::param(*dev, "speed", speed));
    ASSERT_EQ((int)speed, 400000);
}

TEST(driver_core, an_interface_is_named_the_way_the_declarations_name_it)
{
    pdiutil::string out;

    ASSERT_TRUE(DeviceManager::busName(SERIAL_IFACE_I2C, out));
    ASSERT_STREQ(out.c_str(), "i2c0");
    ASSERT_TRUE(DeviceManager::busName(SERIAL_IFACE_I2C1, out));
    ASSERT_STREQ(out.c_str(), "i2c1");
    ASSERT_TRUE(DeviceManager::busName(SERIAL_IFACE_SPI1, out));
    ASSERT_STREQ(out.c_str(), "spi1");
    ASSERT_TRUE(DeviceManager::busName(SERIAL_IFACE_I2S, out));
    ASSERT_STREQ(out.c_str(), "i2s0");
    ASSERT_TRUE(DeviceManager::busName(SERIAL_IFACE_USB_HOST, out));
    ASSERT_STREQ(out.c_str(), "usb0");
    ASSERT_TRUE(DeviceManager::busName(SERIAL_IFACE_UART1, out));
    ASSERT_STREQ(out.c_str(), "uart1");
    ASSERT_FALSE(DeviceManager::busName(SERIAL_IFACE_CMD, out));
}

TEST(driver_core, a_device_is_named_the_way_linux_names_it)
{
    driver_device_t dev;

    dev.m_bus = SERIAL_IFACE_I2C;
    dev.m_target = 0x50;
    ASSERT_EQ(idOf(dev), std::string("0-0050"));

    dev.m_bus = SERIAL_IFACE_I2C1;
    dev.m_target = 0x76;
    ASSERT_EQ(idOf(dev), std::string("1-0076"));

    dev.m_bus = SERIAL_IFACE_SPI;
    dev.m_target = 5;
    ASSERT_EQ(idOf(dev), std::string("spi0.5"));

    dev.m_bus = SERIAL_IFACE_USB_HOST;
    dev.m_target = 2;
    ASSERT_EQ(idOf(dev), std::string("1-2"));

    dev.m_bus = SERIAL_IFACE_I2S;
    dev.m_target = 1;
    ASSERT_EQ(idOf(dev), std::string("i2s0.1"));
}

TEST(driver_core, a_registered_interface_reports_its_own_kind)
{
    Bench bench;

    ASSERT_EQ((int)bench.m_i2c.type(), (int)SERIAL_IFACE_I2C);
    ASSERT_EQ((int)bench.m_usb.type(), (int)SERIAL_IFACE_USB_HOST);

    FakeI2c loose;
    ASSERT_EQ((int)loose.type(), (int)SERIAL_IFACE_MAX);
}

#ifdef ENABLE_I2S_BUS
TEST(driver_core, inmp441_binds_to_a_declared_mic_whose_samples_change)
{
    Bench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5 rate=8000\n");
    DeviceManager mgr;

    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2S, 0);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_EQ((int)bench.m_i2s.m_config.m_bclk, 3);
    ASSERT_EQ((int)bench.m_i2s.m_config.m_ws, 4);
    ASSERT_EQ((int)bench.m_i2s.m_config.m_din, 5);
    ASSERT_EQ((int)bench.m_i2s.m_config.m_dout, -1);
    ASSERT_EQ((int)bench.m_i2s.m_config.m_rate, 8000);
    ASSERT_TRUE(bench.m_i2s.m_config.m_capture);
    ASSERT_EQ((int)dev->m_instance->driverClass(), (int)DRIVER_CLASS_SOUND_CAPTURE);
}

TEST(driver_core, inmp441_takes_the_default_rate_when_none_is_declared)
{
    Bench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5\n");
    DeviceManager mgr;

    mgr.trigger();

    ASSERT_EQ((int)bench.m_i2s.m_config.m_rate, SOUND_DEFAULT_RATE);
}

TEST(driver_core, inmp441_refuses_a_flat_signal_and_frees_the_bus)
{
    Bench bench;
    bench.m_i2s.m_noise = false;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5\n");
    DeviceManager mgr;

    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2S, 0);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_FAILED);
    ASSERT_FALSE(bench.m_i2s.m_configured);
    ASSERT_EQ(bench.m_i2s.m_releases, 1);
}

TEST(driver_core, inmp441_gives_back_the_code_the_bus_refused_with)
{
    Bench bench;
    bench.m_i2s.m_configure_result = PDI_ERR_BUSY;
    driver_device_t dev;
    ASSERT_TRUE(DeviceManager::parseLine("i2s0 rx inmp441 sck=3 ws=4 sd=5", 31, dev));
    Inmp441Driver mic;

    ASSERT_EQ((int)mic.probe(dev, &bench.m_i2s), (int)PDI_ERR_BUSY);
    ASSERT_FALSE(bench.m_i2s.m_configured);
}

TEST(driver_core, inmp441_gives_back_a_read_error_rather_than_calling_the_mic_silent)
{
    Bench bench;
    bench.m_i2s.m_read_result = PDI_ERR_IO;
    driver_device_t dev;
    ASSERT_TRUE(DeviceManager::parseLine("i2s0 rx inmp441 sck=3 ws=4 sd=5", 31, dev));
    Inmp441Driver mic;

    ASSERT_EQ((int)mic.probe(dev, &bench.m_i2s), (int)PDI_ERR_IO);
    ASSERT_FALSE(bench.m_i2s.m_configured);
}

TEST(driver_core, inmp441_waits_out_the_mic_start_up_before_judging_it)
{
    Bench bench;
    bench.m_i2s.m_flat_reads = 20;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5\n");
    DeviceManager mgr;

    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2S, 0);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_EQ((int)bench.m_i2s.timeout(), 0);
}

TEST(driver_core, inmp441_without_its_pins_is_refused_before_the_bus_is_touched)
{
    Bench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4\n");
    DeviceManager mgr;

    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2S, 0);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_FAILED);
    ASSERT_EQ(bench.m_i2s.m_configures, 0);
}

TEST(driver_core, inmp441_is_not_offered_a_playback_line)
{
    Bench bench;
    writeDevicesConf("i2s0 tx inmp441 sck=3 ws=4 sd=5\n");
    DeviceManager mgr;

    mgr.trigger();

    ASSERT_EQ((int)mgr.find(SERIAL_IFACE_I2S, 1)->m_state, (int)DEVICE_STATE_FAILED);
    ASSERT_EQ(bench.m_i2s.m_configures, 0);
}

TEST(driver_core, inmp441_refuses_a_pin_no_board_has_or_a_bus_without_three_default_pins)
{
    Bench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=200 ws=4 sd=5\n");
    DeviceManager mgr;
    mgr.trigger();
    ASSERT_EQ((int)mgr.find(SERIAL_IFACE_I2S, 0)->m_result, (int)PDI_ERR_INVALID_ARG);

    bench.m_i2s.m_defaults = {6, 7};
    writeDevicesConf("i2s0 rx inmp441\n");
    mgr.trigger();
    ASSERT_EQ((int)mgr.find(SERIAL_IFACE_I2S, 0)->m_result, (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ(bench.m_i2s.m_configures, 0);
}

TEST(driver_core, inmp441_takes_reads_too_short_to_hold_two_samples_as_no_signal)
{
    Bench bench;
    bench.m_i2s.m_read_cap = 2;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5\n");
    DeviceManager mgr;

    mgr.trigger();

    ASSERT_EQ((int)mgr.find(SERIAL_IFACE_I2S, 0)->m_result, (int)PDI_ERR_NOT_FOUND);
    ASSERT_FALSE(bench.m_i2s.m_configured);
}

TEST(driver_core, inmp441_dropped_while_it_holds_the_bus_frees_it)
{
    Bench bench;
    driver_device_t dev;
    const char *line = "i2s0 rx inmp441 sck=3 ws=4 sd=5";
    ASSERT_TRUE(DeviceManager::parseLine(line, (uint16_t)strlen(line), dev));
    Inmp441Driver *mic = pdiutil::safe_new<Inmp441Driver>();
    ASSERT_EQ((int)mic->probe(dev, &bench.m_i2s), (int)PDI_OK);
    ASSERT_TRUE(bench.m_i2s.m_configured);

    pdiutil::safe_delete(mic);

    ASSERT_FALSE(bench.m_i2s.m_configured);
    ASSERT_EQ(bench.m_i2s.m_releases, 1);
}

TEST(driver_core, a_mic_taken_out_of_the_declarations_frees_the_bus)
{
    Bench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5\n");
    DeviceManager mgr;
    mgr.trigger();
    ASSERT_TRUE(bench.m_i2s.m_configured);

    writeDevicesConf("");
    mgr.trigger();

    ASSERT_FALSE(bench.m_i2s.m_configured);
    ASSERT_NULL(mgr.find(SERIAL_IFACE_I2S, 0));
}

TEST(driver_core, a_bound_mic_reads_through_its_driver)
{
    Bench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5\n");
    DeviceManager mgr;
    mgr.trigger();
    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2S, 0);
    ASSERT_NOT_NULL(dev);
    ASSERT_NOT_NULL(dev->m_instance);

    uint8_t buf[64] = {0};
    ASSERT_EQ((int)dev->m_instance->readNode(buf, sizeof(buf)), 64);

    pdiutil::string node;
    ASSERT_TRUE(dev->m_instance->nodeName(node));
    ASSERT_STREQ(node.c_str(), DEVICE_NODE_PCM_CAPTURE);
}
#endif

#ifdef ENABLE_I2C_BUS
namespace
{

const char *const EEPROM_LINE = "i2c0 0x50 at24 chip=24c128 size=16384 page=64\n";
const char *const EEPROM_FILE = "/sys/bus/i2c/devices/0-0050/eeprom";

/**
 * An i2c eeprom: a memory reached by a one or two byte word address, with
 * the block bits of a small chip in its bus address, that stores a write
 * inside one page, wrapping at the page edge, and answers no address for a
 * while after each write. The test can unplug it or make the bus fail.
 */
class FakeEeprom : public iI2cSerialInterface
{
public:
    uint32_t m_base = 0x50;
    uint32_t m_size = 16384;
    uint32_t m_page = 64;
    bool m_present = true;
    int m_busy_after_write = 2;
    int m_busy = 0;
    int32_t m_fail = 0;
    bool m_crossed_page = false;
    uint32_t m_largest_write = 0;
    std::vector<uint32_t> m_write_targets;
    std::vector<uint32_t> m_read_targets;
    std::vector<uint8_t> m_mem;
    std::function<void()> m_on_busy;
    std::function<void()> m_on_read;
    int m_reads = 0;
    uint32_t m_largest_read = 0;
    int m_vanish_after_pages = -1;

    uint32_t addressBytes() const { return m_size > AT24_ONE_BYTE_ADDRESS_MAX ? 2 : 1; }

    uint32_t blocks() const
    {
        uint32_t reach = (1 == addressBytes()) ? 256 : 65536;
        return (m_size + reach - 1) / reach;
    }

    bool answers(uint32_t target) const { return m_present && target >= m_base && target < m_base + blocks(); }

    std::vector<uint8_t> &mem()
    {
        if (m_mem.size() != m_size)
        {
            m_mem.assign(m_size, 0xFF);
        }
        return m_mem;
    }

    void fill()
    {
        for (uint32_t i = 0; i < m_size; i++)
        {
            mem()[i] = (uint8_t)(i * 7 + i / 256);
        }
    }

    bool stillBusy()
    {
        if (m_busy <= 0)
        {
            return false;
        }
        m_busy--;
        if (m_on_busy)
        {
            m_on_busy();
        }
        return true;
    }

    bool probe(uint32_t target) override
    {
        return answers(target) && !stillBusy();
    }

    int32_t transfer(const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen) override
    {
        if (m_vanish_after_pages >= 0 && (int)m_write_targets.size() >= m_vanish_after_pages)
        {
            m_present = false;
        }
        if (!answers(m_selected) || stillBusy())
        {
            return PDI_ERR_NOT_FOUND;
        }
        if (0 != m_fail)
        {
            return m_fail;
        }

        uint32_t used = 0;
        uint32_t ab = addressBytes();
        if (nullptr != tx && txlen >= ab)
        {
            uint32_t word = (2 == ab) ? (((uint32_t)tx[0] << 8) | tx[1]) : tx[0];
            uint32_t block = m_selected - m_base;
            m_pointer = (block * ((1 == ab) ? 256 : 65536) + word) % m_size;
            used = ab;
        }

        uint32_t data = (nullptr == tx) ? 0 : txlen - used;
        if (data > 0)
        {
            uint32_t start = m_pointer;
            uint32_t pagebase = start - start % m_page;
            if (start % m_page + data > m_page)
            {
                m_crossed_page = true;
            }
            if (data > m_largest_write)
            {
                m_largest_write = data;
            }
            for (uint32_t i = 0; i < data; i++)
            {
                mem()[pagebase + (start % m_page + i) % m_page] = tx[used + i];
            }
            m_write_targets.push_back(m_selected);
            m_busy = m_busy_after_write;
        }

        for (uint32_t i = 0; i < rxlen && nullptr != rx; i++)
        {
            rx[i] = mem()[(m_pointer + i) % m_size];
        }
        m_pointer = (m_pointer + rxlen) % m_size;
        if (rxlen > 0)
        {
            m_reads++;
            m_read_targets.push_back(m_selected);
            if (rxlen > m_largest_read)
            {
                m_largest_read = rxlen;
            }
            if (m_on_read)
            {
                m_on_read();
            }
        }
        return (int32_t)(rxlen > 0 ? rxlen : txlen);
    }

private:
    uint32_t m_pointer = 0;
};

/**
 * The bench, with the eeprom on the i2c interface in place of the chip bus.
 */
struct EepromBench : Bench
{
    FakeEeprom m_eeprom;

    EepromBench() { iSerialInterface::instances[SERIAL_IFACE_I2C] = &m_eeprom; }
};

/**
 * The serial service's bench, with the eeprom declared and on the i2c
 * interface, taken back off before the eeprom goes.
 */
struct EepromServiceBench : ServiceBench
{
    FakeEeprom m_eeprom;

    EepromServiceBench()
    {
        iSerialInterface::instances[SERIAL_IFACE_I2C] = &m_eeprom;
        writeDevicesConf(EEPROM_LINE);
        if (nullptr != m_mgr)
        {
            m_mgr->trigger();
        }
    }

    ~EepromServiceBench()
    {
        writeDevicesConf(nullptr);
        if (nullptr != m_mgr)
        {
            m_mgr->trigger();
        }
        iSerialInterface::instances[SERIAL_IFACE_I2C] = &m_i2c;
    }
};

driver_device_t eepromDevice(const char *line)
{
    driver_device_t dev;
    uint16_t len = (uint16_t)strlen(line);
    if (len > 0 && '\n' == line[len - 1])
    {
        len--;
    }
    DeviceManager::parseLine(line, len, dev);
    return dev;
}

/**
 * The bound driver of the eeprom at 0x50, as the memory interface it presents.
 */
iNvmemDriverInterface *boundEeprom(DeviceManager &mgr)
{
    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x50);
    if (nullptr == dev || nullptr == dev->m_instance || DRIVER_CLASS_NVMEM != dev->m_instance->driverClass())
    {
        return nullptr;
    }
    return static_cast<iNvmemDriverInterface *>(dev->m_instance);
}

std::string readEepromFile(int &total, uint64_t offset = 0)
{
    std::string got;
    total = __i_fs.readFile(EEPROM_FILE, 16, [&](char *data, uint32_t size) -> bool {
        got.append(data, size);
        return true;
    }, offset);
    return got;
}

} // namespace

TEST(driver_core, at24_binds_to_a_declared_eeprom_with_its_size_and_page)
{
    EepromBench bench;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;

    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x50);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_STREQ(dev->m_driver.c_str(), DRIVER_NAME_AT24);
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);
    ASSERT_EQ((int)eeprom->size(), 16384);
}

TEST(driver_core, at24_binds_without_the_chip_label)
{
    EepromBench bench;
    writeDevicesConf("i2c0 0x50 at24 size=16384 page=64\n");
    DeviceManager mgr;

    mgr.trigger();

    ASSERT_NOT_NULL(boundEeprom(mgr));
}

TEST(driver_core, at24_without_its_size_or_page_is_refused_as_an_invalid_argument)
{
    EepromBench bench;
    driver_device_t nosize = eepromDevice("i2c0 0x50 at24 chip=24c128 page=64");
    driver_device_t nopage = eepromDevice("i2c0 0x50 at24 chip=24c128 size=16384");
    At24Driver first, second;

    ASSERT_EQ((int)first.probe(nosize, &bench.m_eeprom), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ((int)second.probe(nopage, &bench.m_eeprom), (int)PDI_ERR_INVALID_ARG);
}

TEST(driver_core, at24_refuses_a_size_or_page_no_eeprom_has_as_out_of_range)
{
    EepromBench bench;
    const char *lines[] = {
        "i2c0 0x50 at24 size=16000 page=64",
        "i2c0 0x50 at24 size=128 page=256",
        "i2c0 0x50 at24 size=65536 page=128",
        "i2c0 0x50 at24 size=16384 page=0",
        "i2c0 0x50 at24 size=1048576 page=64",
    };

    for (const char *line : lines)
    {
        driver_device_t dev = eepromDevice(line);
        At24Driver eeprom;
        ASSERT_EQ((int)eeprom.probe(dev, &bench.m_eeprom), (int)PDI_ERR_RANGE);
    }
}

TEST(driver_core, at24_with_nothing_at_the_address_is_refused_as_not_found)
{
    EepromBench bench;
    bench.m_eeprom.m_present = false;
    driver_device_t dev = eepromDevice(EEPROM_LINE);
    At24Driver eeprom;

    ASSERT_EQ((int)eeprom.probe(dev, &bench.m_eeprom), (int)PDI_ERR_NOT_FOUND);
}

TEST(driver_core, at24_gives_back_the_code_its_first_read_failed_with)
{
    EepromBench bench;
    bench.m_eeprom.m_fail = PDI_ERR_BUSY;
    driver_device_t dev = eepromDevice(EEPROM_LINE);
    At24Driver eeprom;

    ASSERT_EQ((int)eeprom.probe(dev, &bench.m_eeprom), (int)PDI_ERR_BUSY);
}

TEST(driver_core, at24_is_not_offered_a_bus_of_another_kind)
{
    EepromBench bench;
    driver_device_t dev = eepromDevice(EEPROM_LINE);
    At24Driver eeprom;

    ASSERT_EQ((int)eeprom.probe(dev, &bench.m_i2s), (int)PDI_ERR_NOT_SUPPORTED);
    ASSERT_EQ((int)eeprom.probe(dev, nullptr), (int)PDI_ERR_NOT_SUPPORTED);
}

TEST(driver_core, at24_reads_what_the_chip_holds_from_any_offset)
{
    EepromBench bench;
    bench.m_eeprom.fill();
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);

    uint8_t buf[300] = {0};
    ASSERT_EQ((int)eeprom->readAt(1000, buf, sizeof(buf)), 300);

    for (uint32_t i = 0; i < sizeof(buf); i++)
    {
        ASSERT_EQ((int)buf[i], (int)bench.m_eeprom.mem()[1000 + i]);
    }
}

TEST(driver_core, at24_reads_nothing_past_the_end_and_stops_a_read_at_it)
{
    EepromBench bench;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);

    uint8_t buf[10] = {0};
    ASSERT_EQ((int)eeprom->readAt(16384, buf, sizeof(buf)), 0);
    ASSERT_EQ((int)eeprom->readAt(16380, buf, sizeof(buf)), 4);
}

TEST(driver_core, at24_never_lets_a_write_cross_a_page)
{
    EepromBench bench;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);

    uint8_t data[200];
    for (uint32_t i = 0; i < sizeof(data); i++)
    {
        data[i] = (uint8_t)(i + 1);
    }

    ASSERT_EQ((int)eeprom->writeAt(60, data, sizeof(data)), 200);

    ASSERT_FALSE(bench.m_eeprom.m_crossed_page);
    for (uint32_t i = 0; i < sizeof(data); i++)
    {
        ASSERT_EQ((int)bench.m_eeprom.mem()[60 + i], (int)data[i]);
    }
}

TEST(driver_core, at24_writes_no_more_than_the_page_declared_at_once)
{
    EepromBench bench;
    writeDevicesConf("i2c0 0x50 at24 size=16384 page=32\n");
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);

    uint8_t data[128] = {0};
    ASSERT_EQ((int)eeprom->writeAt(0, data, sizeof(data)), 128);

    ASSERT_EQ((int)bench.m_eeprom.m_largest_write, 32);
}

TEST(driver_core, at24_reaches_a_small_chip_past_its_first_256_bytes_through_the_bus_address)
{
    EepromBench bench;
    bench.m_eeprom.m_size = 512;
    bench.m_eeprom.m_page = 16;
    writeDevicesConf("i2c0 0x50 at24 chip=24c04 size=512 page=16\n");
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);

    const uint8_t data[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT_EQ((int)eeprom->writeAt(300, data, sizeof(data)), 4);

    ASSERT_EQ((int)bench.m_eeprom.m_write_targets.back(), 0x51);
    uint8_t back[4] = {0};
    ASSERT_EQ((int)eeprom->readAt(300, back, sizeof(back)), 4);
    ASSERT_EQ(0, memcmp(back, data, sizeof(data)));
    ASSERT_EQ((int)bench.m_eeprom.mem()[300], 0xDE);
}

TEST(driver_core, at24_splits_a_read_of_a_small_chip_at_each_256_byte_block)
{
    EepromBench bench;
    bench.m_eeprom.m_size = 512;
    bench.m_eeprom.m_page = 16;
    bench.m_eeprom.fill();
    writeDevicesConf("i2c0 0x50 at24 chip=24c04 size=512 page=16\n");
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);
    bench.m_eeprom.m_read_targets.clear();
    uint8_t back[20] = {0};

    ASSERT_EQ((int)eeprom->readAt(250, back, sizeof(back)), 20);

    ASSERT_EQ(0, memcmp(back, bench.m_eeprom.mem().data() + 250, sizeof(back)));
    ASSERT_EQ((int)bench.m_eeprom.m_read_targets.size(), 2);
    ASSERT_EQ((int)bench.m_eeprom.m_read_targets[0], 0x50);
    ASSERT_EQ((int)bench.m_eeprom.m_read_targets[1], 0x51);
}

TEST(driver_core, at24_and_inmp441_not_yet_probed_refuse_as_not_initialized)
{
    At24Driver eeprom;
    uint8_t buf[4] = {0};
    ASSERT_EQ((int)eeprom.readAt(0, buf, sizeof(buf)), (int)PDI_ERR_NOT_INITIALIZED);
    ASSERT_EQ((int)eeprom.writeAt(0, buf, sizeof(buf)), (int)PDI_ERR_NOT_INITIALIZED);
#ifdef ENABLE_I2S_BUS
    Inmp441Driver mic;
    ASSERT_EQ((int)mic.readNode(buf, sizeof(buf)), (int)PDI_ERR_NOT_INITIALIZED);
#endif
}

TEST(driver_core, at24_waits_for_the_chip_to_finish_storing_each_page)
{
    EepromBench bench;
    bench.m_eeprom.m_busy_after_write = 5;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);

    uint8_t data[100] = {0};
    ASSERT_EQ((int)eeprom->writeAt(0, data, sizeof(data)), 100);

    ASSERT_EQ(bench.m_eeprom.m_busy, 0);
}

TEST(driver_core, at24_gives_up_on_a_page_the_chip_never_finishes_storing)
{
    EepromBench bench;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);
    bench.m_eeprom.m_busy_after_write = 1 << 30;

    uint8_t data[8] = {0};
    ASSERT_EQ((int)eeprom->writeAt(0, data, sizeof(data)), (int)PDI_ERR_TIMEOUT);
}

TEST(driver_core, at24_gives_back_the_code_the_bus_failed_a_transfer_with)
{
    EepromBench bench;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);
    uint8_t data[8] = {0};

    bench.m_eeprom.m_fail = PDI_ERR_IO;
    ASSERT_EQ((int)eeprom->writeAt(0, data, sizeof(data)), (int)PDI_ERR_IO);
    ASSERT_EQ((int)eeprom->readAt(0, data, sizeof(data)), (int)PDI_ERR_IO);

    bench.m_eeprom.m_fail = PDI_ERR_BUSY;
    ASSERT_EQ((int)eeprom->writeAt(0, data, sizeof(data)), (int)PDI_ERR_BUSY);

    bench.m_eeprom.m_fail = 0;
    bench.m_eeprom.m_present = false;
    ASSERT_EQ((int)eeprom->writeAt(0, data, sizeof(data)), (int)PDI_ERR_NOT_FOUND);
    ASSERT_EQ((int)eeprom->readAt(0, data, sizeof(data)), (int)PDI_ERR_NOT_FOUND);
}

TEST(driver_core, at24_refuses_a_write_that_starts_at_the_end_and_cuts_one_that_runs_past_it)
{
    EepromBench bench;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);
    uint8_t data[10] = {0};

    ASSERT_EQ((int)eeprom->writeAt(16384, data, sizeof(data)), (int)PDI_ERR_NO_SPACE);
    ASSERT_EQ((int)eeprom->writeAt(16380, data, sizeof(data)), 4);
}

TEST(driver_core, at24_refuses_a_missing_buffer)
{
    EepromBench bench;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);

    ASSERT_EQ((int)eeprom->readAt(0, nullptr, 4), (int)PDI_ERR_NULL_PTR);
    ASSERT_EQ((int)eeprom->writeAt(0, nullptr, 4), (int)PDI_ERR_NULL_PTR);
}

TEST(driver_core, a_scan_while_the_eeprom_stores_a_page_does_not_take_it_for_gone)
{
    EepromBench bench;
    bench.m_eeprom.m_busy_after_write = 3;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);
    int scans = 0;
    bench.m_eeprom.m_on_busy = [&]() {
        if (0 == scans++)
        {
            mgr.scan();
        }
    };

    uint8_t data[8] = {0};
    ASSERT_EQ((int)eeprom->writeAt(0, data, sizeof(data)), 8);

    ASSERT_EQ(scans > 0, true);
    ASSERT_EQ((int)mgr.find(SERIAL_IFACE_I2C, 0x50)->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_EQ(g_removed_events, 0);
}

TEST(driver_core, a_scan_while_the_eeprom_is_read_does_not_let_it_go_until_the_read_ends)
{
    EepromBench bench;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);
    int scans = 0;
    bench.m_eeprom.m_on_read = [&]() {
        if (0 == scans++)
        {
            bench.m_eeprom.m_present = false;
            mgr.scan();
        }
    };

    uint8_t buf[300] = {0};
    ASSERT_EQ((int)eeprom->readAt(0, buf, sizeof(buf)), (int)PDI_ERR_NOT_FOUND);

    ASSERT_EQ((int)mgr.find(SERIAL_IFACE_I2C, 0x50)->m_state, (int)DEVICE_STATE_BOUND);
    mgr.scan();
    ASSERT_EQ((int)mgr.find(SERIAL_IFACE_I2C, 0x50)->m_state, (int)DEVICE_STATE_ABSENT);
}

TEST(driver_core, a_declaration_change_while_the_eeprom_is_busy_waits_until_it_is_done)
{
    EepromBench bench;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iNvmemDriverInterface *eeprom = boundEeprom(mgr);
    ASSERT_NOT_NULL(eeprom);
    int reads = 0;
    bench.m_eeprom.m_on_read = [&]() {
        if (0 == reads++)
        {
            writeDevicesConf("");
            mgr.scan();
            mgr.trigger();
        }
    };

    uint8_t buf[300] = {0};
    ASSERT_EQ((int)eeprom->readAt(0, buf, sizeof(buf)), 300);

    ASSERT_NOT_NULL(boundEeprom(mgr));
    ASSERT_TRUE(mgr.find(SERIAL_IFACE_I2C, 0x50)->m_declared);
    bench.m_eeprom.m_on_read = nullptr;
    mgr.scan();
    ASSERT_NULL(boundEeprom(mgr));
    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x50);
    ASSERT_TRUE(nullptr == dev || !dev->m_declared);
}

TEST(driver_core, a_long_write_to_the_eeprom_file_goes_a_transfer_at_a_time)
{
    EepromServiceBench bench;
    std::string data(1000, 'z');

    ASSERT_EQ(__i_fs.writeFile(EEPROM_FILE, data.c_str(), (uint32_t)data.size()), 1000);

    ASSERT_EQ(0, memcmp(bench.m_eeprom.mem().data(), data.data(), data.size()));
    ASSERT_TRUE(bench.m_eeprom.m_largest_write <= (uint32_t)I2C_TRANSFER_MAX);
}

TEST(driver_core, a_write_to_the_eeprom_file_that_loses_the_chip_between_pages_says_so)
{
    EepromServiceBench bench;
    std::string data(1000, 'z');
    bench.m_eeprom.m_vanish_after_pages = 3;

    ASSERT_EQ(__i_fs.writeFile(EEPROM_FILE, data.c_str(), (uint32_t)data.size()), (int)PDI_ERR_NOT_FOUND);
    ASSERT_EQ((int)bench.m_eeprom.m_write_targets.size(), 3);
}

TEST(driver_core, a_chip_that_goes_while_storing_a_page_ends_the_write_as_timed_out)
{
    EepromServiceBench bench;
    std::string data(1000, 'z');
    int stores = 0;
    bench.m_eeprom.m_busy_after_write = 1;
    bench.m_eeprom.m_on_busy = [&]() {
        if (3 == ++stores)
        {
            bench.m_eeprom.m_present = false;
        }
    };

    ASSERT_EQ(__i_fs.writeFile(EEPROM_FILE, data.c_str(), (uint32_t)data.size()), (int)PDI_ERR_TIMEOUT);
}

TEST(driver_core, a_transfer_to_an_address_goes_to_that_address_whatever_was_selected)
{
    EepromBench bench;
    bench.m_eeprom.fill();
    bench.m_eeprom.select(0x77);
    uint8_t word[2] = {0, 10};
    uint8_t got = 0;

    ASSERT_EQ((int)bench.m_eeprom.transferTo(0x50, word, 2, &got, 1), 1);

    ASSERT_EQ((int)got, (int)bench.m_eeprom.mem()[10]);
}

TEST(driver_core, an_eeprom_unplugged_is_let_go_on_the_next_scan)
{
    EepromBench bench;
    writeDevicesConf(EEPROM_LINE);
    DeviceManager mgr;
    mgr.trigger();
    ASSERT_NOT_NULL(boundEeprom(mgr));

    bench.m_eeprom.m_present = false;
    mgr.scan();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_I2C, 0x50);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_ABSENT);
    ASSERT_NULL(dev->m_instance);
}

TEST(driver_core, a_bound_eeprom_has_an_eeprom_file_the_size_of_the_chip_only_root_may_open)
{
    EepromServiceBench bench;
    pditest::Shell sh;

    std::string out = sh.run("ls /sys/bus/i2c/devices/0-0050");
    ASSERT_TRUE(pditest::saw(out, "eeprom"));
    ASSERT_TRUE(pditest::saw(out, "-rw-------"));
    ASSERT_TRUE(pditest::saw(out, "16384"));
    ASSERT_EQ((int)__i_fs.getFileSize(EEPROM_FILE), 16384);
}

TEST(driver_core, a_device_whose_driver_is_no_memory_has_no_eeprom_file)
{
    ServiceBench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    bench.scan();
    pditest::Shell sh;

    ASSERT_FALSE(pditest::saw(sh.run("ls /sys/bus/i2c/devices/0-0050"), "eeprom"));
    ASSERT_FALSE(__i_fs.isFileExist(EEPROM_FILE));
}

TEST(driver_core, reading_the_eeprom_file_gives_the_whole_chip_or_the_rest_from_an_offset)
{
    EepromServiceBench bench;
    bench.m_eeprom.fill();
    int total = 0;

    std::string whole = readEepromFile(total);
    ASSERT_EQ(total, 16384);
    ASSERT_EQ((int)whole.size(), 16384);
    ASSERT_EQ(0, memcmp(whole.data(), bench.m_eeprom.mem().data(), 16384));

    std::string rest = readEepromFile(total, 16000);
    ASSERT_EQ(total, 384);
    ASSERT_EQ(0, memcmp(rest.data(), bench.m_eeprom.mem().data() + 16000, 384));
}

TEST(driver_core, reading_the_eeprom_file_stops_ahead_of_a_match_or_where_the_reader_says)
{
    EepromServiceBench bench;
    const char text[] = "first line\nsecond";
    memcpy(bench.m_eeprom.mem().data(), text, sizeof(text) - 1);
    std::string got;
    bool matched = false;

    int total = __i_fs.readFile(EEPROM_FILE, 4, [&](char *data, uint32_t size) -> bool {
        got.append(data, size);
        return true;
    }, 0, "\n", &matched);

    ASSERT_EQ(total, 10);
    ASSERT_STREQ(got.c_str(), "first line");
    ASSERT_TRUE(matched);

    int pieces = 0;
    total = __i_fs.readFile(EEPROM_FILE, 4, [&](char *, uint32_t) -> bool { return ++pieces < 2; });
    ASSERT_EQ(pieces, 2);
    ASSERT_EQ(total, 8);
}

TEST(driver_core, lsblk_and_blkid_pass_over_a_bound_device_that_is_not_a_disk)
{
    EepromServiceBench bench;
    ASSERT_NOT_NULL(boundEeprom(*bench.m_mgr));
    pditest::Shell sh;

    std::string list = sh.run("lsblk");
    ASSERT_TRUE(pditest::saw(list, "NAME"));
    ASSERT_FALSE(pditest::saw(list, "0-0050"));
    ASSERT_FALSE(pditest::saw(list, "disk"));
    ASSERT_FALSE(pditest::saw(sh.run("blkid"), "/dev/"));
}

TEST(driver_core, sys_bus_lists_the_drivers_of_a_bus_and_a_devices_target)
{
    EepromServiceBench bench;
    pditest::Shell sh;

    std::string drivers = sh.run("ls /sys/bus/i2c/drivers");
    ASSERT_TRUE(pditest::saw(drivers, "at24"));
    ASSERT_TRUE(pditest::saw(drivers, "testchip"));
    ASSERT_FALSE(pditest::saw(drivers, "mmc_spi"));
    ASSERT_TRUE(__i_fs.isDirExist("/sys/bus/i2c/drivers/at24"));
    ASSERT_FALSE(__i_fs.isDirExist("/sys/bus/i2c/drivers/nosuch"));
    ASSERT_TRUE(pditest::saw(sh.run("cat /sys/bus/i2c/devices/0-0050/target"), "80"));
}

TEST(driver_core, reading_the_eeprom_file_takes_the_chip_a_full_transfer_at_a_time_in_the_reader_chunks)
{
    EepromServiceBench bench;
    bench.m_eeprom.m_reads = 0;
    bench.m_eeprom.m_largest_read = 0;
    uint32_t largest_chunk = 0;

    int total = __i_fs.readFile(EEPROM_FILE, 16, [&](char *data, uint32_t size) -> bool {
        if (size > largest_chunk)
        {
            largest_chunk = size;
        }
        return true;
    });

    ASSERT_EQ(total, 16384);
    ASSERT_EQ((int)largest_chunk, 16);
    ASSERT_EQ((int)bench.m_eeprom.m_largest_read, I2C_TRANSFER_MAX);
    ASSERT_EQ(bench.m_eeprom.m_reads, 16384 / I2C_TRANSFER_MAX);
}

TEST(driver_core, a_read_of_the_eeprom_file_that_loses_the_chip_says_so)
{
    EepromServiceBench bench;
    int chunks = 0;

    int total = __i_fs.readFile(EEPROM_FILE, 16, [&](char *data, uint32_t size) -> bool {
        if (1 == ++chunks)
        {
            bench.m_eeprom.m_present = false;
        }
        return true;
    });

    ASSERT_EQ(total, (int)PDI_ERR_NOT_FOUND);
}

TEST(driver_core, writing_the_eeprom_file_stores_from_its_start)
{
    EepromServiceBench bench;
    pditest::Shell sh;

    ASSERT_EQ(__i_fs.writeFile(EEPROM_FILE, "hello", 5), 5);
    ASSERT_EQ(0, memcmp(bench.m_eeprom.mem().data(), "hello", 5));

    sh.run("echo howdy > /sys/bus/i2c/devices/0-0050/eeprom");
    ASSERT_EQ(0, memcmp(bench.m_eeprom.mem().data(), "howdy", 5));
}

TEST(driver_core, editing_the_eeprom_file_writes_at_the_offset_given)
{
    EepromServiceBench bench;

    ASSERT_EQ(__i_fs.editFile(EEPROM_FILE, 4096, "abc", 3), 3);

    ASSERT_EQ(0, memcmp(bench.m_eeprom.mem().data() + 4096, "abc", 3));
}

TEST(driver_core, appending_to_the_eeprom_file_is_not_supported)
{
    EepromServiceBench bench;

    ASSERT_EQ(__i_fs.writeFile(EEPROM_FILE, "x", 1, true), (int)PDI_ERR_NOT_SUPPORTED);
}
#endif

#ifdef ENABLE_SPI_BUS
namespace
{

const char *const SD_LINE = "spi0 cs=12 mmc_spi\n";
const char *const SD_NODE = "/dev/mmcblk0";
const uint32_t SD_BLOCKS = 8192;

/**
 * The bench with a card behind chip select 12, high capacity and 4 MB unless
 * the test says otherwise.
 */
struct SdBench : Bench
{
    SdCardSim *m_card;

    explicit SdBench(bool highcapacity = true, uint32_t blocks = SD_BLOCKS)
        : m_card(pdiutil::safe_new<SdCardSim>(highcapacity, blocks))
    {
        m_spi.m_cards[12] = m_card;
    }

    ~SdBench()
    {
        m_spi.m_cards.clear();
        pdiutil::safe_delete(m_card);
    }
};

/**
 * The serial service's bench with the card declared, taken back off before
 * the card goes.
 */
struct SdServiceBench : ServiceBench
{
    SdCardSim *m_card;

    SdServiceBench() : m_card(pdiutil::safe_new<SdCardSim>(true, SD_BLOCKS))
    {
        m_spi.m_cards[12] = m_card;
        writeDevicesConf(SD_LINE);
        if (nullptr != m_mgr)
        {
            m_mgr->trigger();
        }
    }

    ~SdServiceBench()
    {
        writeDevicesConf(nullptr);
        if (nullptr != m_mgr)
        {
            m_mgr->trigger();
        }
        m_spi.m_cards.clear();
        pdiutil::safe_delete(m_card);
    }
};

/**
 * The bound driver of the card at chip select 12, as the disk it presents.
 */
iBlockDriverInterface *boundCard(DeviceManager &mgr)
{
    const driver_device_t *dev = mgr.find(SERIAL_IFACE_SPI, 12);
    if (nullptr == dev || nullptr == dev->m_instance || DRIVER_CLASS_BLOCK != dev->m_instance->driverClass())
    {
        return nullptr;
    }
    return static_cast<iBlockDriverInterface *>(dev->m_instance);
}

pdi_err_t probeCard(SdBench &bench, iSerialInterface *bus)
{
    driver_device_t dev;
    DeviceManager::parseLine(SD_LINE, (uint16_t)(strlen(SD_LINE) - 1), dev);
    MmcSpiDriver card;
    return card.probe(dev, bus);
}

void fillCard(SdCardSim *card)
{
    for (uint32_t i = 0; i < card->blocks() * SD_BLOCK_SIZE; i++)
    {
        card->data()[i] = (uint8_t)(i * 13 + i / SD_BLOCK_SIZE);
    }
}

} // namespace

TEST(driver_core, mmc_spi_binds_a_high_capacity_card_and_gives_its_size)
{
    SdBench bench;
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;

    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_SPI, 12);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_STREQ(dev->m_driver.c_str(), DRIVER_NAME_MMC_SPI);
    iBlockDriverInterface *card = boundCard(mgr);
    ASSERT_NOT_NULL(card);
    ASSERT_EQ((int)card->blocks(), (int)SD_BLOCKS);
}

TEST(driver_core, mmc_spi_reads_and_writes_a_standard_capacity_card_by_byte_address)
{
    SdBench bench(false, 4096);
    fillCard(bench.m_card);
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iBlockDriverInterface *card = boundCard(mgr);
    ASSERT_NOT_NULL(card);
    ASSERT_EQ((int)card->blocks(), 4096);

    uint8_t block[SD_BLOCK_SIZE];
    ASSERT_EQ((int)card->readBlocks(3, block, 1), 1);
    ASSERT_EQ(0, memcmp(block, bench.m_card->data() + 3 * SD_BLOCK_SIZE, SD_BLOCK_SIZE));

    memset(block, 0xA5, sizeof(block));
    ASSERT_EQ((int)card->writeBlocks(5, block, 1), 1);
    ASSERT_EQ(0, memcmp(block, bench.m_card->data() + 5 * SD_BLOCK_SIZE, SD_BLOCK_SIZE));
}

TEST(driver_core, mmc_spi_binds_an_older_card_that_does_not_know_cmd8)
{
    SdBench bench(false, 4096);
    bench.m_card->m_v1 = true;
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;

    mgr.trigger();

    ASSERT_NOT_NULL(boundCard(mgr));
}

TEST(driver_core, mmc_spi_reads_a_card_that_does_not_know_cmd59_without_checking_its_data_crc)
{
    SdBench bench;
    fillCard(bench.m_card);
    bench.m_card->m_no_crc_command = true;
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iBlockDriverInterface *card = boundCard(mgr);
    ASSERT_NOT_NULL(card);

    uint8_t block[SD_BLOCK_SIZE];
    ASSERT_EQ((int)card->readBlocks(2, block, 1), 1);
    ASSERT_EQ(0, memcmp(block, bench.m_card->data() + 2 * SD_BLOCK_SIZE, SD_BLOCK_SIZE));
}

TEST(driver_core, mmc_spi_wakes_the_card_slowly_with_its_select_high_then_runs_it_fast)
{
    SdBench bench;
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iBlockDriverInterface *card = boundCard(mgr);
    ASSERT_NOT_NULL(card);

    ASSERT_TRUE(bench.m_spi.m_idle_bytes >= 10);
    ASSERT_EQ((int)bench.m_spi.m_begins.front().first, -1);
    ASSERT_TRUE(bench.m_spi.m_begins.front().second <= (uint32_t)SD_INIT_CLOCK_HZ);
    ASSERT_FALSE(bench.m_spi.m_open);

    uint8_t block[SD_BLOCK_SIZE];
    bench.m_spi.m_begins.clear();
    ASSERT_EQ((int)card->readBlocks(0, block, 1), 1);
    bool selected = false;
    for (const auto &begun : bench.m_spi.m_begins)
    {
        if (12 == begun.first)
        {
            selected = true;
            ASSERT_EQ((int)begun.second, (int)bench.m_spi.clockFor(SD_CLOCK_HZ));
        }
    }
    ASSERT_TRUE(selected);
    ASSERT_FALSE(bench.m_spi.m_open);
}

TEST(driver_core, mmc_spi_with_no_card_is_absent_as_not_found)
{
    SdBench bench;
    bench.m_spi.m_cards.clear();
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;

    mgr.trigger();

    const driver_device_t *dev = mgr.find(SERIAL_IFACE_SPI, 12);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_ABSENT);
    ASSERT_EQ((int)dev->m_result, (int)PDI_ERR_NOT_FOUND);
}

namespace
{

/**
 * How many times the warning log holds the text.
 */
int warningsSaying(const char *text)
{
    std::string log;
    __i_fs.beginPrivileged();
    __i_fs.readFile(SYSLOG_FILE_WARNING, 64, [&](char *data, uint32_t size) -> bool {
        log.append(data, size);
        return true;
    });
    __i_fs.endPrivileged();
    int count = 0;
    for (size_t at = log.find(text); std::string::npos != at; at = log.find(text, at + 1))
    {
        count++;
    }
    return count;
}

} // namespace

TEST(driver_core, a_card_taken_out_is_let_go_on_the_next_scan)
{
    SdServiceBench bench;
    ASSERT_NOT_NULL(boundCard(*bench.m_mgr));
    int removed = g_removed_events;

    bench.m_card->m_present = false;
    bench.m_mgr->scan();

    const driver_device_t *dev = bench.m_mgr->find(SERIAL_IFACE_SPI, 12);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_ABSENT);
    ASSERT_NULL(dev->m_instance);
    ASSERT_EQ(g_removed_events, removed + 1);
    ASSERT_FALSE(__i_fs.isFileExist(SD_NODE));
    ASSERT_FALSE(pditest::saw(pditest::Shell().run("lsblk"), "mmcblk0"));
}

TEST(driver_core, a_card_put_back_is_bound_on_the_next_scan_with_no_trigger)
{
    SdServiceBench bench;
    fillCard(bench.m_card);
    bench.m_card->m_present = false;
    bench.m_mgr->scan();
    int added = g_added_events;

    bench.m_card->reinsert();
    bench.m_mgr->scan();

    iBlockDriverInterface *card = boundCard(*bench.m_mgr);
    ASSERT_NOT_NULL(card);
    ASSERT_EQ(g_added_events, added + 1);
    uint8_t block[SD_BLOCK_SIZE];
    ASSERT_EQ((int)card->readBlocks(4, block, 1), 1);
    ASSERT_EQ(0, memcmp(block, bench.m_card->data() + 4 * SD_BLOCK_SIZE, SD_BLOCK_SIZE));
}

TEST(driver_core, a_card_swapped_between_two_scans_is_let_go_and_bound_again_in_one)
{
    SdServiceBench bench;
    uint32_t before = boundCard(*bench.m_mgr)->serial();
    int removed = g_removed_events;
    int added = g_added_events;

    bench.m_card->reinsert();
    bench.m_mgr->scan();

    iBlockDriverInterface *card = boundCard(*bench.m_mgr);
    ASSERT_NOT_NULL(card);
    ASSERT_NE(card->serial(), before);
    ASSERT_EQ(g_removed_events, removed + 1);
    ASSERT_EQ(g_added_events, added + 1);
}

TEST(driver_core, a_card_that_misses_one_status_answer_stays_bound)
{
    SdServiceBench bench;
    uint32_t before = boundCard(*bench.m_mgr)->serial();

    bench.m_card->m_silent_command = 13;
    bench.m_card->m_silent_count = 1;
    bench.m_mgr->scan();
    ASSERT_EQ(boundCard(*bench.m_mgr)->serial(), before);

    bench.m_spi.m_fail_begin = (int)bench.m_spi.m_begins.size() + 1;
    bench.m_mgr->scan();
    ASSERT_EQ(boundCard(*bench.m_mgr)->serial(), before);
    bench.m_spi.m_fail_begin = 0;

    bench.m_card->m_silent_count = -1;
    bench.m_card->m_present = false;
    bench.m_mgr->scan();
    ASSERT_NULL(boundCard(*bench.m_mgr));
}

TEST(driver_core, an_empty_slot_stays_absent_logs_once_and_binds_the_card_put_in)
{
    ServiceBench bench;
    __syslog_service.initService();
    int before = warningsSaying("refused by mmc_spi");
    writeDevicesConf(SD_LINE);
    bench.m_mgr->trigger();
    int logged = warningsSaying("refused by mmc_spi");

    for (int i = 0; i < 5; i++)
    {
        bench.m_mgr->scan();
    }
    int after = warningsSaying("refused by mmc_spi");
    __syslog_service.stopService();
    ASSERT_EQ(logged, before + 1);
    ASSERT_EQ(after, logged);
    ASSERT_EQ((int)bench.m_mgr->find(SERIAL_IFACE_SPI, 12)->m_state, (int)DEVICE_STATE_ABSENT);

    SdCardSim *card = pdiutil::safe_new<SdCardSim>(true, SD_BLOCKS);
    bench.m_spi.m_cards[12] = card;
    bench.m_mgr->scan();
    ASSERT_NOT_NULL(boundCard(*bench.m_mgr));

    writeDevicesConf(nullptr);
    bench.m_mgr->trigger();
    bench.m_spi.m_cards.clear();
    pdiutil::safe_delete(card);
}

/**
 * A slot with no card-detect line is polled, so an init that went wrong once,
 * a card half way in or a garbled answer from an empty slot, is tried again on
 * the next pass rather than left failed until someone runs udevadm trigger.
 */
TEST(driver_core, a_slot_refused_once_is_tried_again_and_binds_the_card_put_in)
{
    ServiceBench bench;
    __syslog_service.initService();
    SdCardSim *garbled = pdiutil::safe_new<SdCardSim>(true, SD_BLOCKS);
    garbled->m_bad_echo = true;
    bench.m_spi.m_cards[12] = garbled;
    int before = warningsSaying("refused by mmc_spi");
    writeDevicesConf(SD_LINE);
    bench.m_mgr->trigger();
    ASSERT_EQ((int)bench.m_mgr->find(SERIAL_IFACE_SPI, 12)->m_state, (int)DEVICE_STATE_FAILED);

    for (int i = 0; i < 5; i++)
    {
        bench.m_mgr->scan();
    }
    int logged = warningsSaying("refused by mmc_spi");
    ASSERT_EQ(logged, before + 1);

    bench.m_spi.m_cards.clear();
    pdiutil::safe_delete(garbled);
    bench.m_mgr->scan();
    ASSERT_EQ((int)bench.m_mgr->find(SERIAL_IFACE_SPI, 12)->m_state, (int)DEVICE_STATE_ABSENT);

    SdCardSim *card = pdiutil::safe_new<SdCardSim>(true, SD_BLOCKS);
    bench.m_spi.m_cards[12] = card;
    bench.m_mgr->scan();
    __syslog_service.stopService();
    ASSERT_NOT_NULL(boundCard(*bench.m_mgr));

    writeDevicesConf(nullptr);
    bench.m_mgr->trigger();
    bench.m_spi.m_cards.clear();
    pdiutil::safe_delete(card);
}

namespace
{

/**
 * A log that takes long to write, the way a flash append can.
 */
void slowLogSink(logger_type_t type, const char *line, uint16_t len)
{
    (void)type; (void)line; (void)len;
    __i_dvc_ctrl.advanceVirtualClock((uint64_t)(DEVICE_SCAN_PROBE_MAX_MS + 50) * 1000);
}

} // namespace

/**
 * The scan budget is for the probe. Writing down a refusal is the log's time,
 * and a slow log must not take a card slot out of the scan.
 */
TEST(driver_core, a_refusal_written_to_a_slow_log_does_not_leave_the_slot_out_of_the_scan)
{
    ServiceBench bench;
    SdCardSim *garbled = pdiutil::safe_new<SdCardSim>(true, SD_BLOCKS);
    garbled->m_bad_echo = true;
    bench.m_spi.m_cards[12] = garbled;
    writeDevicesConf(SD_LINE);
    bench.m_mgr->trigger();

    __i_dvc_ctrl.useVirtualClock(true);
    __log_manager.setSyslogSink(slowLogSink);
    bench.m_spi.m_cards.clear();
    pdiutil::safe_delete(garbled);
    bench.m_mgr->scan();
    __log_manager.setSyslogSink(nullptr);
    __i_dvc_ctrl.useVirtualClock(false);

    const driver_device_t *dev = bench.m_mgr->find(SERIAL_IFACE_SPI, 12);
    ASSERT_FALSE(dev->m_unscanned);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_ABSENT);

    writeDevicesConf(nullptr);
    bench.m_mgr->trigger();
}

TEST(driver_core, mmc_spi_gives_up_on_a_card_that_never_gets_ready)
{
    SdBench bench;
    bench.m_card->m_never_ready = true;

    ASSERT_EQ((int)probeCard(bench, &bench.m_spi), (int)PDI_ERR_TIMEOUT);
}

TEST(driver_core, mmc_spi_refuses_an_mmc_card_as_not_supported)
{
    SdBench bench;
    bench.m_card->m_mmc = true;

    ASSERT_EQ((int)probeCard(bench, &bench.m_spi), (int)PDI_ERR_NOT_SUPPORTED);
}

TEST(driver_core, mmc_spi_refuses_a_card_that_does_not_accept_the_voltage_as_not_supported)
{
    SdBench bench;
    bench.m_card->m_wrong_voltage = true;

    ASSERT_EQ((int)probeCard(bench, &bench.m_spi), (int)PDI_ERR_NOT_SUPPORTED);
}

TEST(driver_core, mmc_spi_refuses_an_ultra_capacity_card_whose_size_it_cannot_read)
{
    SdBench bench;
    bench.m_card->m_ultra_capacity = true;

    ASSERT_EQ((int)probeCard(bench, &bench.m_spi), (int)PDI_ERR_NOT_SUPPORTED);
}

TEST(driver_core, mmc_spi_is_not_offered_a_bus_of_another_kind)
{
    SdBench bench;

    ASSERT_EQ((int)probeCard(bench, &bench.m_i2c), (int)PDI_ERR_NOT_SUPPORTED);
    ASSERT_EQ((int)probeCard(bench, nullptr), (int)PDI_ERR_NOT_SUPPORTED);
}

TEST(driver_core, mmc_spi_refuses_blocks_past_the_end_and_a_missing_buffer)
{
    SdBench bench;
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iBlockDriverInterface *card = boundCard(mgr);
    ASSERT_NOT_NULL(card);
    uint8_t block[SD_BLOCK_SIZE];

    ASSERT_EQ((int)card->readBlocks(SD_BLOCKS, block, 1), (int)PDI_ERR_NO_SPACE);
    ASSERT_EQ((int)card->readBlocks(SD_BLOCKS - 1, block, 2), (int)PDI_ERR_NO_SPACE);
    ASSERT_EQ((int)card->writeBlocks(SD_BLOCKS, block, 1), (int)PDI_ERR_NO_SPACE);
    ASSERT_EQ((int)card->readBlocks(0, nullptr, 1), (int)PDI_ERR_NULL_PTR);
    ASSERT_EQ((int)card->writeBlocks(0, nullptr, 1), (int)PDI_ERR_NULL_PTR);
}

TEST(driver_core, mmc_spi_names_each_way_a_read_can_fail)
{
    SdBench bench;
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iBlockDriverInterface *card = boundCard(mgr);
    ASSERT_NOT_NULL(card);
    uint8_t block[SD_BLOCK_SIZE];

    bench.m_card->m_read_error = 0x08;
    ASSERT_EQ((int)card->readBlocks(0, block, 1), (int)PDI_ERR_IO);

    bench.m_card->m_read_error = 0;
    bench.m_card->m_bad_crc = true;
    ASSERT_EQ((int)card->readBlocks(0, block, 1), (int)PDI_ERR_CRC);

    bench.m_card->m_bad_crc = false;
    bench.m_card->m_present = false;
    ASSERT_EQ((int)card->readBlocks(0, block, 1), (int)PDI_ERR_NOT_FOUND);
}

TEST(driver_core, mmc_spi_names_each_way_a_write_can_fail)
{
    SdBench bench;
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iBlockDriverInterface *card = boundCard(mgr);
    ASSERT_NOT_NULL(card);
    uint8_t block[SD_BLOCK_SIZE] = {0};

    bench.m_card->m_write_response = 0x0B;
    ASSERT_EQ((int)card->writeBlocks(0, block, 1), (int)PDI_ERR_CRC);

    bench.m_card->m_write_response = 0x0D;
    ASSERT_EQ((int)card->writeBlocks(0, block, 1), (int)PDI_ERR_IO);

    bench.m_card->m_write_response = 0x05;
    bench.m_card->m_never_done = true;
    ASSERT_EQ((int)card->writeBlocks(0, block, 1), (int)PDI_ERR_TIMEOUT);
    ASSERT_EQ((int)card->readBlocks(0, block, 1), (int)PDI_ERR_TIMEOUT);

    bench.m_card->m_never_done = false;
    bench.m_card->m_present = false;
    ASSERT_EQ((int)card->writeBlocks(0, block, 1), (int)PDI_ERR_NOT_FOUND);
}

TEST(driver_core, mmc_spi_waits_out_a_card_that_stays_busy_before_it_is_ready)
{
    SdBench bench;
    bench.m_card->m_ready_after = 200;

    ASSERT_EQ((int)probeCard(bench, &bench.m_spi), (int)PDI_OK);
}

TEST(driver_core, mmc_spi_names_a_start_up_command_the_card_leaves_unanswered_as_not_found)
{
    const int high[] = {0, 8, 59, 55, 41, 58, 9};
    for (int index : high)
    {
        SdBench bench;
        bench.m_card->m_silent_command = index;
        ASSERT_EQ((int)probeCard(bench, &bench.m_spi), (int)PDI_ERR_NOT_FOUND);
    }

    SdBench standard(false, 4096);
    standard.m_card->m_silent_command = 16;
    ASSERT_EQ((int)probeCard(standard, &standard.m_spi), (int)PDI_ERR_NOT_FOUND);
}

TEST(driver_core, mmc_spi_names_a_slot_that_never_answers_reset_as_idle_as_not_found)
{
    SdBench bench;
    bench.m_card->m_refused_command = 0;

    ASSERT_EQ((int)probeCard(bench, &bench.m_spi), (int)PDI_ERR_NOT_FOUND);
}

TEST(driver_core, mmc_spi_names_a_start_up_command_the_card_refuses_as_io)
{
    const int high[] = {8, 59, 58, 9};
    for (int index : high)
    {
        SdBench bench;
        bench.m_card->m_refused_command = index;
        ASSERT_EQ((int)probeCard(bench, &bench.m_spi), (int)PDI_ERR_IO);
    }

    SdBench standard(false, 4096);
    standard.m_card->m_refused_command = 16;
    ASSERT_EQ((int)probeCard(standard, &standard.m_spi), (int)PDI_ERR_IO);
}

TEST(driver_core, mmc_spi_refuses_a_card_that_echoes_its_check_pattern_wrongly_as_io)
{
    SdBench bench;
    bench.m_card->m_bad_echo = true;

    ASSERT_EQ((int)probeCard(bench, &bench.m_spi), (int)PDI_ERR_IO);
}

TEST(driver_core, mmc_spi_times_out_on_a_card_that_never_sends_its_data)
{
    SdBench refused;
    refused.m_card->m_no_token = true;
    ASSERT_EQ((int)probeCard(refused, &refused.m_spi), (int)PDI_ERR_TIMEOUT);

    SdBench bench;
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iBlockDriverInterface *card = boundCard(mgr);
    ASSERT_NOT_NULL(card);
    uint8_t block[SD_BLOCK_SIZE];
    bench.m_card->m_no_token = true;

    ASSERT_EQ((int)card->readBlocks(0, block, 1), (int)PDI_ERR_TIMEOUT);
    bench.m_card->m_no_token = false;
    ASSERT_EQ((int)card->readBlocks(0, block, 1), 1);
}

TEST(driver_core, mmc_spi_names_a_write_the_card_never_answers_as_not_found_and_carries_on)
{
    SdBench bench;
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iBlockDriverInterface *card = boundCard(mgr);
    ASSERT_NOT_NULL(card);
    uint8_t block[SD_BLOCK_SIZE];
    memset(block, 0x5A, sizeof(block));
    bench.m_card->m_no_write_response = true;

    ASSERT_EQ((int)card->writeBlocks(1, block, 1), (int)PDI_ERR_NOT_FOUND);
    ASSERT_NE(bench.m_card->data()[SD_BLOCK_SIZE], 0x5A);
    bench.m_card->m_no_write_response = false;
    ASSERT_EQ((int)card->writeBlocks(1, block, 1), 1);
    ASSERT_EQ(bench.m_card->data()[SD_BLOCK_SIZE], 0x5A);
}

TEST(driver_core, mmc_spi_gives_back_the_code_the_bus_refused_any_selection_with)
{
    SdBench clean;
    ASSERT_EQ((int)probeCard(clean, &clean.m_spi), (int)PDI_OK);
    std::vector<std::pair<int16_t, uint32_t>> begins = clean.m_spi.m_begins;
    ASSERT_TRUE(begins.size() > 4);

    for (size_t k = 1; k <= begins.size(); k++)
    {
        SdBench bench;
        bench.m_spi.m_fail_begin = (int)k;
        bool selecting = (1 == k) || (12 == begins[k - 1].first);
        ASSERT_EQ((int)probeCard(bench, &bench.m_spi), selecting ? (int)PDI_ERR_BUSY : (int)PDI_OK);
    }
}

TEST(driver_core, mmc_spi_gives_back_the_code_the_bus_refused_a_block_with)
{
    SdBench bench;
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iBlockDriverInterface *card = boundCard(mgr);
    ASSERT_NOT_NULL(card);
    uint8_t block[SD_BLOCK_SIZE] = {0};

    bench.m_spi.m_fail_begin = (int)bench.m_spi.m_begins.size() + 1;
    ASSERT_EQ((int)card->readBlocks(0, block, 1), (int)PDI_ERR_BUSY);
    bench.m_spi.m_fail_begin = (int)bench.m_spi.m_begins.size() + 1;
    ASSERT_EQ((int)card->writeBlocks(0, block, 1), (int)PDI_ERR_BUSY);
    bench.m_spi.m_fail_begin = 0;
    ASSERT_EQ((int)card->readBlocks(0, block, 1), 1);
}

TEST(driver_core, mmc_spi_not_yet_probed_refuses_a_block_as_not_initialized)
{
    MmcSpiDriver idle;
    uint8_t block[SD_BLOCK_SIZE];

    ASSERT_EQ((int)idle.readBlocks(0, block, 1), (int)PDI_ERR_NOT_INITIALIZED);
    ASSERT_EQ((int)idle.writeBlocks(0, block, 1), (int)PDI_ERR_NOT_INITIALIZED);
}

TEST(driver_core, mmc_spi_writes_and_reads_back_several_blocks)
{
    SdBench bench;
    writeDevicesConf(SD_LINE);
    DeviceManager mgr;
    mgr.trigger();
    iBlockDriverInterface *card = boundCard(mgr);
    ASSERT_NOT_NULL(card);
    std::vector<uint8_t> out(3 * SD_BLOCK_SIZE), back(3 * SD_BLOCK_SIZE);
    for (uint32_t i = 0; i < out.size(); i++)
    {
        out[i] = (uint8_t)(i * 7);
    }

    ASSERT_EQ((int)card->writeBlocks(100, out.data(), 3), 3);
    ASSERT_EQ((int)card->readBlocks(100, back.data(), 3), 3);

    ASSERT_EQ(0, memcmp(out.data(), back.data(), out.size()));
    ASSERT_EQ(bench.m_card->m_writes, 3);
}

TEST(driver_core, a_bound_card_is_a_disk_under_dev_only_root_may_open)
{
    SdServiceBench bench;
    pditest::Shell sh;

    std::string out = sh.run("ls /dev");
    ASSERT_TRUE(pditest::saw(out, "mmcblk0"));
    ASSERT_TRUE(pditest::saw(out, "-rw-------"));
    ASSERT_TRUE(pditest::saw(out, "4194304"));
    ASSERT_EQ((int)__i_fs.getFileSize(SD_NODE), (int)(SD_BLOCKS * SD_BLOCK_SIZE));
}

TEST(driver_core, reading_the_disk_gives_the_card_from_any_offset)
{
    SdServiceBench bench;
    fillCard(bench.m_card);
    std::string got;

    int total = __i_fs.readFile(SD_NODE, 16, [&](char *data, uint32_t size) -> bool {
        got.append(data, size);
        return got.size() < 2000;
    }, 700);

    ASSERT_TRUE(total >= 2000);
    ASSERT_EQ(0, memcmp(got.data(), bench.m_card->data() + 700, 2000));
}

TEST(driver_core, writing_the_disk_at_an_offset_keeps_the_rest_of_its_blocks)
{
    SdServiceBench bench;
    fillCard(bench.m_card);
    std::vector<uint8_t> before(bench.m_card->data(), bench.m_card->data() + 2 * SD_BLOCK_SIZE);

    ASSERT_EQ(__i_fs.editFile(SD_NODE, 510, "abcd", 4), 4);

    ASSERT_EQ(0, memcmp(bench.m_card->data() + 510, "abcd", 4));
    ASSERT_EQ(0, memcmp(bench.m_card->data(), before.data(), 510));
    ASSERT_EQ(0, memcmp(bench.m_card->data() + 514, before.data() + 514, 2 * SD_BLOCK_SIZE - 514));
}

TEST(driver_core, writing_the_disk_stores_from_its_start_and_refuses_an_append)
{
    SdServiceBench bench;
    pditest::Shell sh;

    ASSERT_EQ(__i_fs.writeFile(SD_NODE, "hello", 5), 5);
    ASSERT_EQ(0, memcmp(bench.m_card->data(), "hello", 5));
    ASSERT_EQ(__i_fs.writeFile(SD_NODE, "x", 1, true), (int)PDI_ERR_NOT_SUPPORTED);

    sh.run("echo howdy > /dev/mmcblk0");
    ASSERT_EQ(0, memcmp(bench.m_card->data(), "howdy", 5));
}

TEST(driver_core, head_of_the_disk_prints_the_line_written_to_it)
{
    SdServiceBench bench;
    pditest::Shell sh;

    sh.run("echo hello from the card > /dev/mmcblk0");

    ASSERT_TRUE(pditest::saw(sh.run("head /dev/mmcblk0 1"), "hello from the card"));
}

namespace
{

/**
 * Puts a FAT boot sector at the block given.
 */
void putFatBootSector(SdCardSim *card, uint32_t block)
{
    uint8_t *b = card->data() + block * SD_BLOCK_SIZE;
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    memcpy(b + 3, "MSDOS5.0", 8);
    b[11] = 0x00;
    b[12] = 0x02;
    b[13] = 4;
    b[14] = 1;
    b[16] = 2;
    b[21] = 0xF8;
    memcpy(b + 54, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;
}

/**
 * Puts a dos partition table at block 0 with one partition.
 */
void putPartitionTable(SdCardSim *card, uint32_t start, uint32_t blocks, uint8_t id)
{
    uint8_t *b = card->data();
    uint8_t *entry = b + 446;
    memset(entry, 0, 64);
    entry[0] = 0x00;
    entry[4] = id;
    memcpy(entry + 8, &start, 4);
    memcpy(entry + 12, &blocks, 4);
    b[510] = 0x55;
    b[511] = 0xAA;
}

/**
 * The card bench with the card blanked and bound again, so it is read afresh.
 */
void rebind(SdServiceBench &bench)
{
    writeDevicesConf("");
    bench.m_mgr->trigger();
    writeDevicesConf(SD_LINE);
    bench.m_mgr->trigger();
}

} // namespace

TEST(driver_core, blkid_and_lsblk_name_a_card_formatted_whole)
{
    SdServiceBench bench;
    putFatBootSector(bench.m_card, 0);
    rebind(bench);
    pditest::Shell sh;

    ASSERT_TRUE(pditest::saw(sh.run("blkid"), "/dev/mmcblk0: TYPE=\"vfat\""));
    std::string list = sh.run("lsblk");
    ASSERT_TRUE(pditest::saw(list, "FSTYPE"));
    ASSERT_TRUE(pditest::saw(list, "vfat"));
    ASSERT_FALSE(pditest::saw(sh.run("ls /dev"), "mmcblk0p1"));
}

TEST(driver_core, a_partitioned_card_gives_each_partition_a_node_and_a_sys_block_entry)
{
    SdServiceBench bench;
    putPartitionTable(bench.m_card, 2048, 4096, 0x0C);
    putFatBootSector(bench.m_card, 2048);
    rebind(bench);
    pditest::Shell sh;

    std::string dev = sh.run("ls /dev");
    ASSERT_TRUE(pditest::saw(dev, "mmcblk0p1"));
    ASSERT_TRUE(pditest::saw(dev, "2097152"));
    std::string ids = sh.run("blkid");
    ASSERT_TRUE(pditest::saw(ids, "/dev/mmcblk0: PTTYPE=\"dos\""));
    ASSERT_TRUE(pditest::saw(ids, "/dev/mmcblk0p1: TYPE=\"vfat\""));
    std::string list = sh.run("lsblk");
    ASSERT_TRUE(pditest::saw(list, "mmcblk0p1"));
    ASSERT_TRUE(pditest::saw(list, "part"));
    ASSERT_TRUE(pditest::saw(sh.run("cat /sys/block/mmcblk0/mmcblk0p1/start"), "2048"));
    ASSERT_TRUE(pditest::saw(sh.run("cat /sys/block/mmcblk0/mmcblk0p1/size"), "4096"));
}

TEST(driver_core, a_partition_reads_from_its_first_block_and_stops_at_its_end)
{
    SdServiceBench bench;
    fillCard(bench.m_card);
    putPartitionTable(bench.m_card, 2048, 4096, 0x0C);
    rebind(bench);
    std::string got;

    int total = __i_fs.readFile("/dev/mmcblk0p1", 64, [&](char *data, uint32_t size) -> bool {
        got.append(data, size);
        return got.size() < 1000;
    });

    ASSERT_TRUE(total >= 1000);
    ASSERT_EQ(0, memcmp(got.data(), bench.m_card->data() + 2048 * SD_BLOCK_SIZE, 1000));
    ASSERT_EQ((int)__i_fs.getFileSize("/dev/mmcblk0p1"), 4096 * SD_BLOCK_SIZE);
    ASSERT_EQ(__i_fs.editFile("/dev/mmcblk0p1", 4096 * SD_BLOCK_SIZE, "x", 1), (int)PDI_ERR_NO_SPACE);
    ASSERT_EQ(__i_fs.editFile("/dev/mmcblk0p1", 4096 * SD_BLOCK_SIZE - 2, "abcd", 4), 2);
    ASSERT_EQ(0, memcmp(bench.m_card->data() + 6144 * SD_BLOCK_SIZE - 2, "ab", 2));
}

TEST(driver_core, blkid_names_a_filesystem_it_cannot_mount)
{
    SdServiceBench bench;
    memcpy(bench.m_card->data() + 3, "NTFS    ", 8);
    rebind(bench);
    pditest::Shell sh;

    ASSERT_TRUE(pditest::saw(sh.run("blkid"), "/dev/mmcblk0: TYPE=\"ntfs\""));
}

TEST(driver_core, writing_the_first_blocks_of_a_disk_reads_its_layout_again)
{
    SdServiceBench bench;
    pditest::Shell sh;
    ASSERT_FALSE(pditest::saw(sh.run("blkid"), "mmcblk0"));

    std::string sector(SD_BLOCK_SIZE, '\0');
    SdCardSim scratch(true, 1024);
    putFatBootSector(&scratch, 0);
    memcpy(&sector[0], scratch.data(), SD_BLOCK_SIZE);
    ASSERT_EQ(__i_fs.editFile(SD_NODE, 0, sector.data(), SD_BLOCK_SIZE), SD_BLOCK_SIZE);

    ASSERT_TRUE(pditest::saw(sh.run("blkid"), "/dev/mmcblk0: TYPE=\"vfat\""));
}

TEST(driver_core, dd_copies_a_block_off_the_disk_and_back_onto_it_at_another_place)
{
    SdServiceBench bench;
    fillCard(bench.m_card);
    std::vector<uint8_t> before(bench.m_card->data(), bench.m_card->data() + 8 * SD_BLOCK_SIZE);
    pditest::Shell sh;

    ASSERT_TRUE(pditest::saw(sh.run("dd if=/dev/mmcblk0 of=/blk.bin bs=512 count=1 skip=3"), "1+0 records out"));
    ASSERT_TRUE(pditest::saw(sh.run("dd if=/blk.bin of=/dev/mmcblk0 bs=512 seek=5"), "512 bytes copied"));

    ASSERT_EQ(0, memcmp(bench.m_card->data() + 5 * SD_BLOCK_SIZE, before.data() + 3 * SD_BLOCK_SIZE, SD_BLOCK_SIZE));
    ASSERT_EQ(0, memcmp(bench.m_card->data() + 4 * SD_BLOCK_SIZE, before.data() + 4 * SD_BLOCK_SIZE, SD_BLOCK_SIZE));
    ASSERT_EQ(0, memcmp(bench.m_card->data() + 6 * SD_BLOCK_SIZE, before.data() + 6 * SD_BLOCK_SIZE, SD_BLOCK_SIZE));
    __i_fs.deleteFile("/blk.bin");
}

TEST(driver_core, a_read_of_the_disk_that_loses_the_card_says_so)
{
    SdServiceBench bench;
    int chunks = 0;

    int total = __i_fs.readFile(SD_NODE, 16, [&](char *data, uint32_t size) -> bool {
        if (1 == ++chunks)
        {
            bench.m_card->m_present = false;
        }
        return true;
    });

    ASSERT_EQ(total, (int)PDI_ERR_NOT_FOUND);
}

TEST(driver_core, a_bound_card_gives_its_size_in_sectors_under_sys_block)
{
    SdServiceBench bench;
    pditest::Shell sh;

    ASSERT_TRUE(pditest::saw(sh.run("ls /sys/block"), "mmcblk0"));
    ASSERT_TRUE(pditest::saw(sh.run("cat /sys/block/mmcblk0/size"), "8192"));
}

TEST(driver_core, lsblk_lists_each_disk_with_its_size_and_type)
{
    SdServiceBench bench;
    pditest::Shell sh;

    std::string out = sh.run("lsblk");

    ASSERT_TRUE(pditest::saw(out, "NAME"));
    ASSERT_TRUE(pditest::saw(out, "mmcblk0"));
    ASSERT_TRUE(pditest::saw(out, "4194304"));
    ASSERT_TRUE(pditest::saw(out, "disk"));
}

TEST(driver_core, lsblk_with_no_disk_lists_only_its_header)
{
    ServiceBench bench;
    pditest::Shell sh;

    std::string out = sh.run("lsblk");

    ASSERT_TRUE(pditest::saw(out, "NAME"));
    ASSERT_FALSE(pditest::saw(out, "mmcblk0"));
}

#ifdef ENABLE_FAT_FS
namespace
{

/**
 * Lays an empty FAT12 volume over the blocks given: four blocks a cluster, one
 * reserved block, two FATs and a 512 entry root directory.
 */
void putFatVolume(SdCardSim *card, uint32_t block, uint32_t blocks)
{
    uint8_t *b = card->data() + block * SD_BLOCK_SIZE;
    uint16_t fatblocks = (uint16_t)(((blocks / 4 + 2) * 3 / 2 + SD_BLOCK_SIZE - 1) / SD_BLOCK_SIZE);
    uint16_t total = (uint16_t)blocks;
    memset(b, 0, (1 + 2 * fatblocks + 32) * SD_BLOCK_SIZE);
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    memcpy(b + 3, "MSDOS5.0", 8);
    b[12] = 0x02;
    b[13] = 4;
    b[14] = 1;
    b[16] = 2;
    b[18] = 0x02;
    memcpy(b + 19, &total, 2);
    b[21] = 0xF8;
    memcpy(b + 22, &fatblocks, 2);
    b[38] = 0x29;
    memcpy(b + 43, "NO NAME    ", 11);
    memcpy(b + 54, "FAT12   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;
    for (uint8_t fat = 0; fat < 2; fat++)
    {
        uint8_t *table = b + (1 + fat * fatblocks) * SD_BLOCK_SIZE;
        table[0] = 0xF8;
        table[1] = 0xFF;
        table[2] = 0xFF;
    }
}

/**
 * The card bench with the whole card formatted FAT and /mnt made to mount it on.
 */
struct FatCardBench : SdServiceBench
{
    FatCardBench()
    {
        fillCard(m_card);
        putFatVolume(m_card, 0, SD_BLOCKS);
        rebind(*this);
        __i_fs.createDirectory("/mnt");
    }

    ~FatCardBench()
    {
        __i_fs.umount("/mnt");
        __i_fs.deleteDirectory("/mnt");
    }
};

} // namespace

TEST(driver_core, a_fat_card_mounts_on_a_directory_and_keeps_what_is_written_there)
{
    FatCardBench bench;
    pditest::Shell sh;

    ASSERT_FALSE(pditest::saw(sh.run("mount /dev/mmcblk0 /mnt"), "CmdErr"));
    std::string table = sh.run("mount");
    ASSERT_TRUE(pditest::saw(table, "/mnt"));
    ASSERT_TRUE(pditest::saw(table, "vfat"));
    ASSERT_TRUE(pditest::saw(table, "mmcblk0"));
    sh.run("echo kept on the card > /mnt/note.txt");
    ASSERT_TRUE(pditest::saw(sh.run("cat /mnt/note.txt"), "kept on the card"));

    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_OK);
    ASSERT_FALSE(__i_fs.isFileExist("/mnt/note.txt"));
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);

    ASSERT_TRUE(pditest::saw(sh.run("cat /mnt/note.txt"), "kept on the card"));
}

TEST(driver_core, a_mounted_fat_card_holds_directories_listings_and_sizes)
{
    FatCardBench bench;
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);
    pditest::Shell sh;

    sh.run("mkdir /mnt/logs");
    sh.run("echo first > /mnt/logs/a.log");
    sh.run("echo second line >> /mnt/logs/a.log");
    sh.run("cp /mnt/logs/a.log /mnt/b.log");

    ASSERT_TRUE(__i_fs.isDirExist("/mnt/logs"));
    ASSERT_EQ((int)__i_fs.getFileSize("/mnt/logs/a.log"), 18);
    ASSERT_TRUE(pditest::saw(sh.run("cat /mnt/b.log"), "second line"));
    std::string list = sh.run("ls /mnt");
    ASSERT_TRUE(pditest::saw(list, "logs"));
    ASSERT_TRUE(pditest::saw(list, "b.log"));

    sh.run("rm /mnt/logs");
    ASSERT_FALSE(__i_fs.isDirExist("/mnt/logs"));
    ASSERT_TRUE(__i_fs.isFileExist("/mnt/b.log"));
}

/**
 * The text commands find lines through the filesystem, so the card has to
 * answer those questions the way the flash does.
 */
TEST(driver_core, the_text_commands_read_lines_on_the_card_as_on_the_flash)
{
    FatCardBench bench;
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);
    pditest::Shell sh;
    sh.run("echo hello card > /mnt/t.txt");
    sh.run("echo second line >> /mnt/t.txt");

    std::string last = sh.run("tail /mnt/t.txt 1");
    ASSERT_TRUE(pditest::saw(last, "second line"));
    ASSERT_FALSE(pditest::saw(last, "hello card"));
    std::string first = sh.run("head /mnt/t.txt 1");
    ASSERT_TRUE(pditest::saw(first, "hello card"));
    ASSERT_FALSE(pditest::saw(first, "second line"));
    ASSERT_TRUE(pditest::saw(sh.run("wc /mnt/t.txt"), "2 4 23"));
    ASSERT_TRUE(pditest::saw(sh.run("grep second /mnt/t.txt"), ":2:"));

    sh.run("echo echo ran from the card > /mnt/run.sh");
    sh.run("echo echo second card line >> /mnt/run.sh");
    std::string listed = sh.run("ls /mnt");
    ASSERT_TRUE(pditest::saw(listed, " ..\r") || pditest::saw(listed, " ..\n"));
    ASSERT_TRUE(pditest::saw(listed, " .\r") || pditest::saw(listed, " .\n"));
    std::string ran = sh.run("source /mnt/run.sh");
    ASSERT_TRUE(pditest::saw(ran, "ran from the card"));
    ASSERT_TRUE(pditest::saw(ran, "second card line"));
}

/**
 * A copy off the card is streamed into one open file on the flash, so it has
 * to arrive whole however many chunks it takes, and an empty one has to arrive
 * as an empty file.
 */
TEST(driver_core, a_copy_off_the_card_arrives_whole_on_the_flash_and_in_tmp)
{
    FatCardBench bench;
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);
    pditest::Shell sh;
    std::string bytes;
    for (int i = 0; i < 1000; i++)
    {
        bytes += (char)((i * 7 + 3) & 0xFF);
    }
    __i_fs.createFile("/mnt/blob.bin", bytes.data(), (int64_t)bytes.size());
    __i_fs.createFile("/mnt/empty.bin", "", 0);
    __i_fs.deleteFile("/blob_copy.bin");
    __i_fs.deleteFile("/empty_copy.bin");
    __i_fs.deleteFile("/tmp/blob_copy.bin");

    ASSERT_EQ((int)__i_fs.copyFile("/mnt/blob.bin", "/blob_copy.bin"), 0);
    ASSERT_EQ((int)__i_fs.copyFile("/mnt/empty.bin", "/empty_copy.bin"), 0);
    ASSERT_EQ((int)__i_fs.copyFile("/mnt/blob.bin", "/tmp/blob_copy.bin"), 0);

    std::string flash;
    __i_fs.readFile("/blob_copy.bin", 128, [&flash](char *c, uint32_t n) { flash += std::string(c, n); return true; });
    std::string tmp;
    __i_fs.readFile("/tmp/blob_copy.bin", 128, [&tmp](char *c, uint32_t n) { tmp += std::string(c, n); return true; });
    bool empty = __i_fs.isFileExist("/empty_copy.bin") && 0 == __i_fs.getFileSize("/empty_copy.bin");
    __i_fs.deleteFile("/blob_copy.bin");
    __i_fs.deleteFile("/empty_copy.bin");
    __i_fs.deleteFile("/tmp/blob_copy.bin");

    ASSERT_TRUE(flash == bytes);
    ASSERT_TRUE(tmp == bytes);
    ASSERT_TRUE(empty);
}

TEST(driver_core, a_fat_partition_mounts_and_writes_nothing_outside_it)
{
    SdServiceBench bench;
    fillCard(bench.m_card);
    putPartitionTable(bench.m_card, 2048, 4096, 0x01);
    putFatVolume(bench.m_card, 2048, 4096);
    rebind(bench);
    __i_fs.createDirectory("/mnt");
    std::vector<uint8_t> before(bench.m_card->data(), bench.m_card->data() + SD_BLOCKS * SD_BLOCK_SIZE);

    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0p1", "/mnt"), (int)PDI_OK);
    ASSERT_EQ(__i_fs.writeFile("/mnt/inside.txt", "inside", 6), 6);
    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_OK);
    __i_fs.deleteDirectory("/mnt");

    ASSERT_EQ(0, memcmp(bench.m_card->data(), before.data(), 2048 * SD_BLOCK_SIZE));
    ASSERT_EQ(0, memcmp(bench.m_card->data() + 6144 * SD_BLOCK_SIZE, before.data() + 6144 * SD_BLOCK_SIZE, 2048 * SD_BLOCK_SIZE));
    ASSERT_NE(0, memcmp(bench.m_card->data() + 2048 * SD_BLOCK_SIZE, before.data() + 2048 * SD_BLOCK_SIZE, 4096 * SD_BLOCK_SIZE));
}

TEST(driver_core, mount_refuses_each_thing_it_cannot_mount_with_its_own_code)
{
    FatCardBench bench;

    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/null", "/mnt"), (int)PDI_ERR_NOT_FOUND);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/mnt/x", "/mnt"), (int)PDI_ERR_NOT_FOUND);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/no/such/dir"), (int)STORAGE_ERROR_NOT_A_DIRECTORY);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/tmp"), (int)PDI_ERR_EXISTS);
    __i_fs.createDirectory("/a_mount_point_too_long");
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/a_mount_point_too_long"), (int)PDI_ERR_INVALID_ARG);
    __i_fs.deleteDirectory("/a_mount_point_too_long");
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/tmp"), (int)PDI_ERR_BUSY);
}

TEST(driver_core, mount_names_a_card_that_carries_no_fat_by_the_fatfs_result)
{
    SdServiceBench bench;
    fillCard(bench.m_card);
    rebind(bench);
    __i_fs.createDirectory("/mnt");

    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_ERR_FROM_FAT(FR_NO_FILESYSTEM));
    ASSERT_FALSE(pditest::saw(pditest::Shell().run("mount"), "vfat"));
    __i_fs.deleteDirectory("/mnt");
}

TEST(driver_core, mount_refuses_a_partitioned_disk_as_a_whole_and_takes_its_partition)
{
    SdServiceBench bench;
    fillCard(bench.m_card);
    putPartitionTable(bench.m_card, 2048, 4096, 0x0C);
    putFatVolume(bench.m_card, 2048, 4096);
    rebind(bench);
    __i_fs.createDirectory("/mnt");

    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_ERR_NOT_SUPPORTED);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0p1", "/mnt"), (int)PDI_OK);

    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_OK);
    __i_fs.deleteDirectory("/mnt");
}

TEST(driver_core, mount_refuses_a_filesystem_it_names_but_cannot_mount_as_not_supported)
{
    const char *signatures[] = {"NTFS    ", "EXFAT   "};
    for (const char *sig : signatures)
    {
        SdServiceBench bench;
        memcpy(bench.m_card->data() + 3, sig, 8);
        rebind(bench);
        __i_fs.createDirectory("/mnt");
        ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_ERR_NOT_SUPPORTED);
        __i_fs.deleteDirectory("/mnt");
    }

    SdServiceBench bench;
    putPartitionTable(bench.m_card, 2048, 4096, 0x83);
    memcpy(bench.m_card->data() + 2048 * SD_BLOCK_SIZE + 8, "littlefs", 8);
    rebind(bench);
    __i_fs.createDirectory("/mnt");
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0p1", "/mnt"), (int)PDI_ERR_NOT_SUPPORTED);
    __i_fs.deleteDirectory("/mnt");
}

TEST(driver_core, only_root_may_mount_a_card)
{
    FatCardBench bench;
    pditest::Shell sh(1000, 1000);

    ASSERT_TRUE(pditest::saw(sh.run("mount /dev/mmcblk0 /mnt"), "mount: /dev/mmcblk0 : -12"));
    ASSERT_FALSE(pditest::saw(pditest::Shell().run("mount"), "vfat"));
}

TEST(driver_core, a_card_with_a_file_open_is_not_unmounted_until_it_closes)
{
    FatCardBench bench;
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);

    pdi_fhandle_t handle = __i_fs.openFile("/mnt/open.txt", FILE_OPEN_WRITE | FILE_OPEN_CREATE);
    ASSERT_TRUE(handle >= 0);
    ASSERT_EQ(__i_fs.writeFileHandle(handle, "abc", 3), 3);

    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_ERR_BUSY);
    ASSERT_EQ((int)__i_fs.closeFile(handle), (int)PDI_OK);
    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_OK);
}

TEST(driver_core, a_card_pulled_while_mounted_fails_a_read_with_the_disk_code)
{
    FatCardBench bench;
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);
    ASSERT_EQ(__i_fs.writeFile("/mnt/gone.txt", "gone", 4), 4);
    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_OK);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);
    bench.m_card->m_present = false;

    int total = __i_fs.readFile("/mnt/gone.txt", 16, [](char *, uint32_t) -> bool { return true; });

    ASSERT_EQ(total, (int)PDI_ERR_NOT_FOUND);
    bench.m_card->m_present = true;
}

TEST(driver_core, a_file_on_the_card_is_read_up_to_a_match_in_reader_sized_pieces)
{
    FatCardBench bench;
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);
    ASSERT_EQ(__i_fs.writeFile("/mnt/lines.txt", "alpha\nbeta\ngamma\n", 17), 17);
    std::string got;
    uint32_t largest = 0;
    bool matched = false;

    int total = __i_fs.readFile("/mnt/lines.txt", 3, [&](char *data, uint32_t size) -> bool {
        got.append(data, size);
        largest = (size > largest) ? size : largest;
        return true;
    }, 6, "\ng", &matched);

    ASSERT_EQ(total, 4);
    ASSERT_STREQ(got.c_str(), "beta");
    ASSERT_TRUE(matched);
    ASSERT_TRUE(largest <= 3);
}

namespace
{

/* the bytes one 4 block cluster of the test volumes holds */
const uint32_t FAT_CLUSTER_BYTES = 4 * SD_BLOCK_SIZE;

/* data clusters putFatVolume lays over the whole 8192 block card: (8192 - 45) / 4 */
const uint64_t FAT_CARD_CLUSTERS = 2036;

/* 2025-10-14 00:00:00 UTC, an even second so FAT keeps it exactly */
const uint32_t FAT_TEST_EPOCH = 1760400000u;

/**
 * Puts one entry of a dos partition table, leaving the others as they are.
 */
void putPartitionEntry(SdCardSim *card, uint8_t index, uint32_t start, uint32_t blocks)
{
    uint8_t *entry = card->data() + 446 + index * 16;
    memset(entry, 0, 16);
    entry[4] = 0x01;
    memcpy(entry + 8, &start, 4);
    memcpy(entry + 12, &blocks, 4);
    card->data()[510] = 0x55;
    card->data()[511] = 0xAA;
}

/**
 * The backend mounted at the directory, so sizes are asked of the volume
 * rather than of the root filesystem.
 */
iFileSystemInterface *mountedAt(const char *dir)
{
    const vfs_mount_t *mount = __i_fs.findMountForPath(dir);
    return (nullptr == mount) ? nullptr : mount->m_backend;
}

/**
 * The FAT card bench mounted at /mnt.
 */
struct MountedFatBench : FatCardBench
{
    MountedFatBench() { mounted = FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"); }
    pdi_err_t mounted;
};

/**
 * A clock that reads the test epoch, put back to unset afterwards.
 */
struct FatClock
{
    FatClock() { __i_ntp.set_ntp_time(FAT_TEST_EPOCH); }
    ~FatClock() { __i_ntp.clearTime(); }
};

std::string readAll(const char *path)
{
    std::string got;
    __i_fs.readFile(path, 64, [&](char *data, uint32_t size) -> bool {
        got.append(data, size);
        return true;
    });
    return got;
}

} // namespace

TEST(driver_core, a_mounted_card_gives_its_own_sizes_not_the_root_filesystems)
{
    MountedFatBench bench;
    ASSERT_EQ((int)bench.mounted, (int)PDI_OK);
    iFileSystemInterface *card = mountedAt("/mnt");
    ASSERT_NOT_NULL(card);
    uint64_t total = FAT_CARD_CLUSTERS * FAT_CLUSTER_BYTES;

    ASSERT_EQ(card->getTotalSize(), total);
    ASSERT_EQ(card->getFreeSize(), total);
    ASSERT_EQ(card->getUsedSize(), (uint64_t)0);
    ASSERT_EQ(__i_fs.writeFile("/mnt/one.txt", "0123456789", 10), 10);
    ASSERT_EQ(card->getFreeSize(), total - FAT_CLUSTER_BYTES);
    ASSERT_EQ(card->getUsedSize(), (uint64_t)FAT_CLUSTER_BYTES);
    ASSERT_NE(__i_fs.getTotalSize(), total);

    std::string df = pditest::Shell().run("df");
    ASSERT_TRUE(pditest::saw(df, "/mnt"));
    ASSERT_TRUE(pditest::saw(df, "4169728"));
}

TEST(driver_core, only_root_may_unmount_a_card)
{
    MountedFatBench bench;
    ASSERT_EQ((int)bench.mounted, (int)PDI_OK);

    ASSERT_TRUE(pditest::saw(pditest::Shell(1000, 1000).run("umount /mnt"), "umount: /mnt : -12"));
    ASSERT_TRUE(pditest::saw(pditest::Shell().run("mount"), "vfat"));
    ASSERT_FALSE(pditest::saw(pditest::Shell().run("umount /mnt"), "CmdErr"));
    ASSERT_FALSE(pditest::saw(pditest::Shell().run("mount"), "vfat"));
}

TEST(driver_core, a_file_created_on_the_card_holds_its_content_and_is_not_created_twice)
{
    MountedFatBench bench;

    ASSERT_EQ(__i_fs.createFile("/mnt/new.txt", "fresh"), 5);
    ASSERT_EQ(__i_fs.createFile("/mnt/part.txt", "abcdef", 3), 3);
    ASSERT_EQ(__i_fs.createFile("/mnt/empty.txt", nullptr, 0), 0);

    ASSERT_STREQ(readAll("/mnt/new.txt").c_str(), "fresh");
    ASSERT_STREQ(readAll("/mnt/part.txt").c_str(), "abc");
    ASSERT_EQ((int)__i_fs.getFileSize("/mnt/empty.txt"), 0);
    ASSERT_EQ(__i_fs.createFile("/mnt/new.txt", "again"), (int)PDI_ERR_FROM_FAT(FR_EXIST));
    ASSERT_STREQ(readAll("/mnt/new.txt").c_str(), "fresh");
}

TEST(driver_core, a_write_to_the_card_replaces_or_appends_and_an_empty_one_creates_nothing)
{
    MountedFatBench bench;

    ASSERT_EQ(__i_fs.writeFile("/mnt/w.txt", "first", 5), 5);
    ASSERT_EQ(__i_fs.writeFile("/mnt/w.txt", "second", 6), 6);
    ASSERT_STREQ(readAll("/mnt/w.txt").c_str(), "second");
    ASSERT_EQ(__i_fs.writeFile("/mnt/w.txt", "+more", 5, true), 5);
    ASSERT_STREQ(readAll("/mnt/w.txt").c_str(), "second+more");

    ASSERT_EQ(__i_fs.writeFile("/mnt/none.txt", "x", 0), 0);
    ASSERT_FALSE(__i_fs.isFileExist("/mnt/none.txt"));
    ASSERT_EQ(__i_fs.writeFile("/mnt/null.txt", nullptr, 3), (int)PDI_ERR_NULL_PTR);
}

TEST(driver_core, an_edit_on_the_card_overwrites_in_place_and_pads_a_gap_with_zeros)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/e.txt", "abcdef", 6), 6);

    ASSERT_EQ(__i_fs.editFile("/mnt/e.txt", 2, "XY", 2), 2);
    ASSERT_STREQ(readAll("/mnt/e.txt").c_str(), "abXYef");
    ASSERT_EQ(__i_fs.editFile("/mnt/e.txt", 100, "Z", 1), 1);

    std::string got = readAll("/mnt/e.txt");
    ASSERT_EQ((int)got.size(), 101);
    ASSERT_EQ(got.substr(0, 6), std::string("abXYef"));
    ASSERT_EQ(got.substr(6, 94), std::string(94, '\0'));
    ASSERT_EQ(got[100], 'Z');
    ASSERT_EQ(__i_fs.editFile("/mnt/e.txt", 0, "q", 0), 0);
    ASSERT_EQ(__i_fs.editFile("/mnt/e.txt", 0, nullptr, 1), (int)PDI_ERR_NULL_PTR);
    ASSERT_EQ(__i_fs.editFile("/mnt/missing.txt", 0, "q", 1), (int)PDI_ERR_FROM_FAT(FR_NO_FILE));
}

TEST(driver_core, a_rename_on_the_card_replaces_a_file_and_moves_directories)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/a.txt", "from a", 6), 6);
    ASSERT_EQ(__i_fs.writeFile("/mnt/b.txt", "from b", 6), 6);
    ASSERT_EQ((int)__i_fs.createDirectory("/mnt/d"), (int)PDI_OK);

    ASSERT_EQ((int)__i_fs.rename("/mnt/a.txt", "/mnt/b.txt"), (int)PDI_OK);
    ASSERT_FALSE(__i_fs.isFileExist("/mnt/a.txt"));
    ASSERT_STREQ(readAll("/mnt/b.txt").c_str(), "from a");

    ASSERT_EQ((int)__i_fs.moveFile("/mnt/b.txt", "/mnt/d/b.txt"), (int)PDI_OK);
    ASSERT_STREQ(readAll("/mnt/d/b.txt").c_str(), "from a");
    ASSERT_EQ((int)__i_fs.rename("/mnt/d", "/mnt/e"), (int)PDI_OK);
    ASSERT_TRUE(__i_fs.isFileExist("/mnt/e/b.txt"));
    ASSERT_EQ((int)__i_fs.rename("/mnt/gone", "/mnt/x"), (int)PDI_ERR_FROM_FAT(FR_NO_FILE));
}

TEST(driver_core, deleting_on_the_card_tells_a_file_from_a_directory)
{
    MountedFatBench bench;
    ASSERT_EQ((int)__i_fs.createDirectory("/mnt/p/q/r"), (int)PDI_OK);
    ASSERT_EQ(__i_fs.writeFile("/mnt/p/q/r/f.txt", "f", 1), 1);
    ASSERT_EQ(__i_fs.writeFile("/mnt/p/top.txt", "t", 1), 1);

    ASSERT_TRUE(__i_fs.isDirExist("/mnt/p/q"));
    ASSERT_EQ((int)__i_fs.deleteFile("/mnt/p/q"), (int)STORAGE_ERROR_NOT_A_FILE);
    ASSERT_EQ((int)__i_fs.deleteDirectory("/mnt/p/top.txt"), (int)STORAGE_ERROR_NOT_A_DIRECTORY);
    ASSERT_EQ((int)__i_fs.deleteFile("/mnt/nothing.txt"), (int)PDI_ERR_FROM_FAT(FR_NO_FILE));
    ASSERT_EQ((int)__i_fs.deleteDirectory("/mnt/nothing"), (int)PDI_ERR_FROM_FAT(FR_NO_FILE));
    ASSERT_EQ((int)__i_fs.getFileSize("/mnt/p"), (int)STORAGE_ERROR_NOT_A_FILE);
    ASSERT_EQ((int)__i_fs.getFileSize("/mnt/nothing.txt"), (int)PDI_ERR_FROM_FAT(FR_NO_FILE));

    ASSERT_EQ((int)__i_fs.deleteFile("/mnt/p/top.txt"), (int)PDI_OK);
    ASSERT_EQ((int)__i_fs.deleteDirectory("/mnt/p"), (int)PDI_OK);
    ASSERT_FALSE(__i_fs.isDirExist("/mnt/p"));
    ASSERT_TRUE(__i_fs.isDirExist("/mnt"));
}

TEST(driver_core, a_listing_on_the_card_keeps_only_names_starting_with_the_pattern)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/log1.txt", "1", 1), 1);
    ASSERT_EQ(__i_fs.writeFile("/mnt/log2.txt", "2", 1), 1);
    ASSERT_EQ(__i_fs.writeFile("/mnt/other.txt", "3", 1), 1);
    pdiutil::vector<file_info_t> items;

    ASSERT_EQ(__i_fs.getDirFileList("/mnt", items, "log"), 0);

    ASSERT_EQ((int)items.size(), 2);
    for (uint32_t i = 0; i < items.size(); i++)
    {
        ASSERT_EQ(0, strncmp(items[i].m_name, "log", 3));
        ASSERT_EQ((int)items[i].m_size, 1);
        pdiutil::safe_delete_array(items[i].m_name);
    }
}

TEST(driver_core, a_file_on_the_card_carries_the_time_it_was_written)
{
    MountedFatBench bench;
    {
        FatClock clock;
        ASSERT_EQ(__i_fs.writeFile("/mnt/t.txt", "t", 1), 1);
        file_info_t meta;
        ASSERT_EQ((int)__i_fs.getFileMeta("/mnt/t.txt", meta), (int)PDI_OK);
        ASSERT_EQ(meta.m_mtime, FAT_TEST_EPOCH);
        ASSERT_EQ(meta.m_ctime, FAT_TEST_EPOCH);

        uint32_t later = FAT_TEST_EPOCH + 3600;
        ASSERT_EQ(__i_fs.setFileAttr("/mnt/t.txt", FILE_ATTR_MTIME, &later, sizeof(later)), (int)sizeof(later));
        uint32_t got = 0;
        ASSERT_EQ(__i_fs.getFileAttr("/mnt/t.txt", FILE_ATTR_MTIME, &got, sizeof(got)), (int)sizeof(got));
        ASSERT_EQ(got, later);

        ASSERT_EQ((int)__i_fs.touch("/mnt/t.txt"), (int)PDI_OK);
        ASSERT_EQ(__i_fs.getFileAttr("/mnt/t.txt", FILE_ATTR_MTIME, &got, sizeof(got)), (int)sizeof(got));
        ASSERT_EQ(got, FAT_TEST_EPOCH);
        ASSERT_EQ((int)__i_fs.touch("/mnt/touched.txt"), (int)PDI_OK);
        ASSERT_TRUE(__i_fs.isFileExist("/mnt/touched.txt"));
    }

    ASSERT_EQ(__i_fs.writeFile("/mnt/u.txt", "u", 1), 1);
    file_info_t meta;
    ASSERT_EQ((int)__i_fs.getFileMeta("/mnt/u.txt", meta), (int)PDI_OK);
    ASSERT_EQ(meta.m_mtime, 0u);
    ASSERT_EQ((int)__i_fs.touch("/mnt/u.txt"), (int)PDI_OK);
}

TEST(driver_core, an_attribute_on_the_card_reads_back_what_fat_can_keep)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/a.txt", "a", 1), 1);
    uint16_t value = 0xFFFF;
    uint32_t stamp = 0;

    ASSERT_EQ(__i_fs.getFileAttr("/mnt/a.txt", FILE_ATTR_PERMS, &value, sizeof(value)), (int)sizeof(value));
    ASSERT_EQ((int)value, FAT_PERM_DEFAULT);
    ASSERT_EQ(__i_fs.getFileAttr("/mnt/a.txt", FILE_ATTR_UID, &value, sizeof(value)), (int)sizeof(value));
    ASSERT_EQ((int)value, 0);
    ASSERT_EQ(__i_fs.getFileAttr("/mnt/a.txt", FILE_ATTR_GID, &value, sizeof(value)), (int)sizeof(value));
    ASSERT_EQ((int)value, 0);
    ASSERT_EQ(__i_fs.getFileAttr("/mnt/a.txt", FILE_ATTR_CTIME, &stamp, sizeof(stamp)), (int)sizeof(stamp));
    ASSERT_EQ(__i_fs.getFileAttr("/mnt/a.txt", 200, &value, sizeof(value)), (int)STORAGE_ERROR_ATTR_NOT_FOUND);
    ASSERT_EQ(__i_fs.getFileAttr("/mnt/a.txt", FILE_ATTR_PERMS, nullptr, 2), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ(__i_fs.getFileAttr("/mnt/no.txt", FILE_ATTR_PERMS, &value, sizeof(value)), (int)PDI_ERR_FROM_FAT(FR_NO_FILE));

    ASSERT_EQ(__i_fs.setFileAttr("/mnt/a.txt", FILE_ATTR_UID, &value, sizeof(value)), (int)STORAGE_ERROR_ATTR_NOT_FOUND);
    ASSERT_EQ(__i_fs.setFileAttr("/mnt/a.txt", FILE_ATTR_MTIME, nullptr, 4), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ(__i_fs.removeFileAttr("/mnt/a.txt", FILE_ATTR_MTIME), (int)PDI_ERR_NOT_SUPPORTED);
    ASSERT_EQ(__i_fs.setFileOwner("/mnt/a.txt", 5, 5), (int)PDI_ERR_NOT_SUPPORTED);
}

/**
 * FAT keeps no time zone: an entry's time is the local wall time, the way
 * every other system that reads the card takes it. The framework counts in
 * UTC, so it adds the zone going on and takes it off coming back.
 */
TEST(driver_core, a_time_on_the_card_is_the_local_wall_time)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/t.txt", "t", 1), 1);
    uint32_t utc = 1761143400u;
    ASSERT_EQ(__i_fs.setFileAttr("/mnt/t.txt", FILE_ATTR_MTIME, &utc, sizeof(utc)), (int)sizeof(utc));

    FILINFO info;
    memset(&info, 0, sizeof(info));
    FRESULT fr = f_stat("0:/t.txt", &info);
    if (FR_OK != fr)
    {
        fr = f_stat("1:/t.txt", &info);
    }
    ASSERT_EQ((int)fr, (int)FR_OK);

    datetime_t local;
    EpochToDateTime(utc + (uint32_t)(TZ_SEC + DST_SEC), local);
    ASSERT_EQ((int)(1980 + (info.fdate >> 9)), (int)local.m_year);
    ASSERT_EQ((int)((info.fdate >> 5) & 0x0F), (int)local.m_month);
    ASSERT_EQ((int)(info.fdate & 0x1F), (int)local.m_day);
    ASSERT_EQ((int)(info.ftime >> 11), (int)local.m_hour);
    ASSERT_EQ((int)((info.ftime >> 5) & 0x3F), (int)local.m_minute);

    file_info_t meta;
    ASSERT_EQ((int)__i_fs.getFileMeta("/mnt/t.txt", meta), 0);
    ASSERT_EQ(meta.m_mtime, utc);
}

TEST(driver_core, chmod_on_the_card_sets_and_clears_read_only)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/ro.txt", "keep", 4), 4);
    uint16_t perms = 0444;

    ASSERT_EQ(__i_fs.setFileAttr("/mnt/ro.txt", FILE_ATTR_PERMS, &perms, sizeof(perms)), (int)sizeof(perms));
    file_info_t meta;
    ASSERT_EQ((int)__i_fs.getFileMeta("/mnt/ro.txt", meta), (int)PDI_OK);
    ASSERT_EQ((int)meta.m_perms, FAT_PERM_DEFAULT & ~FAT_PERM_WRITE_BITS);
    ASSERT_EQ(__i_fs.writeFile("/mnt/ro.txt", "lost", 4), (int)PDI_ERR_FROM_FAT(FR_DENIED));
    ASSERT_STREQ(readAll("/mnt/ro.txt").c_str(), "keep");

    pditest::Shell().run("chmod 644 /mnt/ro.txt");
    ASSERT_EQ((int)__i_fs.getFileMeta("/mnt/ro.txt", meta), (int)PDI_OK);
    ASSERT_EQ((int)meta.m_perms, FAT_PERM_DEFAULT);
    ASSERT_EQ(__i_fs.writeFile("/mnt/ro.txt", "free", 4), 4);
    ASSERT_EQ(__i_fs.setFilePermissions("/mnt", 0444), (int)PDI_ERR_NOT_SUPPORTED);
    ASSERT_EQ(__i_fs.setFilePermissions("/mnt/no.txt", 0444), (int)PDI_ERR_FROM_FAT(FR_NO_FILE));
}

TEST(driver_core, a_handle_on_the_card_reads_seeks_appends_and_truncates)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/h.txt", "0123456789", 10), 10);
    char buf[16] = {0};

    pdi_fhandle_t h = __i_fs.openFile("/mnt/h.txt", FILE_OPEN_READ);
    ASSERT_TRUE(h >= 0);
    ASSERT_EQ(__i_fs.readFileHandle(h, buf, 4), 4);
    ASSERT_EQ(std::string(buf, 4), std::string("0123"));
    ASSERT_EQ((int)__i_fs.seekFile(h, 2, FILE_SEEK_CUR), 6);
    ASSERT_EQ(__i_fs.readFileHandle(h, buf, 2), 2);
    ASSERT_EQ(std::string(buf, 2), std::string("67"));
    ASSERT_EQ((int)__i_fs.seekFile(h, -3, FILE_SEEK_END), 7);
    ASSERT_EQ((int)__i_fs.seekFile(h, 1, FILE_SEEK_SET), 1);
    ASSERT_EQ((int)__i_fs.seekFile(h, -5, FILE_SEEK_SET), (int)PDI_ERR_RANGE);
    ASSERT_EQ(__i_fs.readFileHandle(h, buf, 0), 0);
    ASSERT_EQ(__i_fs.readFileHandle(h, nullptr, 4), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ((int)__i_fs.closeFile(h), (int)PDI_OK);

    h = __i_fs.openFile("/mnt/h.txt", FILE_OPEN_WRITE | FILE_OPEN_APPEND);
    ASSERT_TRUE(h >= 0);
    ASSERT_EQ((int)__i_fs.seekFile(h, 0, FILE_SEEK_SET), 0);
    ASSERT_EQ(__i_fs.writeFileHandle(h, "AB", 2), 2);
    ASSERT_EQ(__i_fs.writeFileHandle(h, "AB", 0), 0);
    ASSERT_EQ((int)__i_fs.syncFile(h), (int)PDI_OK);
    ASSERT_EQ((int)__i_fs.closeFile(h), (int)PDI_OK);
    ASSERT_STREQ(readAll("/mnt/h.txt").c_str(), "0123456789AB");

    h = __i_fs.openFile("/mnt/h.txt", FILE_OPEN_WRITE | FILE_OPEN_TRUNCATE);
    ASSERT_TRUE(h >= 0);
    ASSERT_EQ((int)__i_fs.closeFile(h), (int)PDI_OK);
    ASSERT_EQ((int)__i_fs.getFileSize("/mnt/h.txt"), 0);

    ASSERT_EQ((int)__i_fs.openFile("/mnt/absent.txt", FILE_OPEN_READ), (int)PDI_ERR_FROM_FAT(FR_NO_FILE));
}

TEST(driver_core, the_card_refuses_a_handle_past_the_open_file_limit_and_a_handle_it_never_gave)
{
    MountedFatBench bench;
    iFileSystemInterface *card = mountedAt("/mnt");
    pdi_fhandle_t held[VFS_MAX_OPEN_FILES];

    for (uint8_t i = 0; i < VFS_MAX_OPEN_FILES; i++)
    {
        char name[24];
        snprintf(name, sizeof(name), "/f%u.txt", (unsigned)i);
        held[i] = card->openFile(name, FILE_OPEN_WRITE | FILE_OPEN_CREATE);
        ASSERT_TRUE(held[i] >= 0);
    }
    ASSERT_EQ((int)card->openCount(), (int)VFS_MAX_OPEN_FILES);
    ASSERT_EQ((int)card->openFile("/extra.txt", FILE_OPEN_WRITE | FILE_OPEN_CREATE), (int)STORAGE_ERROR_NODE_LIMIT);

    for (uint8_t i = 0; i < VFS_MAX_OPEN_FILES; i++)
    {
        ASSERT_EQ((int)card->closeFile(held[i]), (int)PDI_OK);
    }
    ASSERT_EQ((int)card->closeFile(held[0]), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ((int)card->seekFile(-1, 0, FILE_SEEK_SET), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ((int)card->syncFile(VFS_MAX_OPEN_FILES), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ(card->writeFileHandle(0, "x", 1), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ((int)card->openFile("", FILE_OPEN_READ), (int)STORAGE_ERROR_BAD_PATH);
}

TEST(driver_core, a_read_of_the_card_is_refused_bad_arguments_and_ends_at_the_end)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/r.txt", "0123456789", 10), 10);
    int calls = 0;
    auto count = [&](char *, uint32_t) -> bool { calls++; return false; };

    ASSERT_EQ(__i_fs.readFile("/mnt/r.txt", 4, count, 10), 0);
    ASSERT_EQ(__i_fs.readFile("/mnt/r.txt", 0, count), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ(__i_fs.readFile("/mnt/r.txt", 4, nullptr), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ(calls, 0);
    ASSERT_EQ(__i_fs.readFile("/mnt/r.txt", 4, count), 4);
    ASSERT_EQ(calls, 1);
    ASSERT_EQ(__i_fs.readFile("/mnt/none.txt", 4, count), (int)PDI_ERR_FROM_FAT(FR_NO_FILE));
}

TEST(driver_core, ctrl_c_stops_cp_on_the_card_and_across_mounts_leaving_no_partial_copy)
{
    MountedFatBench bench;
    std::string body(20000, 'c');
    ASSERT_EQ(__i_fs.writeFile("/mnt/src.bin", body.data(), (uint32_t)body.size()), (int)body.size());
    const char *const lines[] = {"cp /mnt/src.bin /mnt/dst.bin\r\x03", "cp /mnt/src.bin /dst.bin\r\x03"};
    const char *const made[] = {"/mnt/dst.bin", "/dst.bin"};

    for (int i = 0; i < 2; i++)
    {
        pditest::Shell sh;
        std::string out = sh.type(lines[i]);
        ASSERT_TRUE(pditest::saw(out, (std::string("CmdErr : ") + std::to_string((int)CMD_ERROR_CANCELED)).c_str()));
        ASSERT_FALSE(__i_fs.isFileExist(made[i]));
    }

    pditest::Shell sh;
    ASSERT_FALSE(pditest::saw(sh.type("cp /mnt/src.bin /dst.bin\r"), "CmdErr"));
    ASSERT_EQ((int)__i_fs.getFileSize("/dst.bin"), (int)body.size());
    __i_fs.deleteFile("/dst.bin");
}

TEST(driver_core, a_full_card_refuses_a_write_and_keeps_no_partial_copy)
{
    SdServiceBench bench;
    fillCard(bench.m_card);
    putPartitionTable(bench.m_card, 2048, 256, 0x01);
    putFatVolume(bench.m_card, 2048, 256);
    rebind(bench);
    __i_fs.createDirectory("/mnt");
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0p1", "/mnt"), (int)PDI_OK);
    uint64_t room = mountedAt("/mnt")->getFreeSize();
    std::string half((size_t)(room / 2 + FAT_CLUSTER_BYTES), 'h');

    ASSERT_EQ(__i_fs.writeFile("/mnt/half.bin", half.data(), (uint32_t)half.size()), (int)half.size());
    ASSERT_EQ((int)__i_fs.copyFile("/mnt/half.bin", "/mnt/copy.bin"), (int)PDI_ERR_NO_SPACE);
    ASSERT_FALSE(__i_fs.isFileExist("/mnt/copy.bin"));
    ASSERT_EQ(__i_fs.writeFile("/mnt/more.bin", half.data(), (uint32_t)half.size()), (int)PDI_ERR_NO_SPACE);
    ASSERT_EQ(__i_fs.editFile("/mnt/half.bin", room * 2, "x", 1), (int)PDI_ERR_NO_SPACE);
    ASSERT_EQ((int)__i_fs.copyFile("/mnt/gone.bin", "/mnt/c.bin"), (int)PDI_ERR_FROM_FAT(FR_NO_FILE));

    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_OK);
    __i_fs.deleteDirectory("/mnt");
}

namespace
{

/**
 * A session the access checks consult, as the user given; with no session the
 * checks see root.
 */
struct UserSession
{
    session_t m_session;
    session_t *m_previous;

    UserSession(uint16_t uid, uint16_t gid) : m_previous(SessionManager::current())
    {
        m_session.m_sid = 98;
        m_session.m_state = SESSION_STATE_INTERACTIVE;
        m_session.m_uid = uid;
        m_session.m_gid = gid;
        SessionManager::setCurrent(&m_session);
    }

    ~UserSession() { SessionManager::setCurrent(m_previous); }
};

} // namespace

TEST(driver_core, only_root_reads_or_writes_the_raw_card_and_its_partitions)
{
    SdServiceBench bench;
    fillCard(bench.m_card);
    putPartitionTable(bench.m_card, 2048, 4096, 0x0C);
    rebind(bench);
    std::vector<uint8_t> card(bench.m_card->data(), bench.m_card->data() + SD_BLOCKS * SD_BLOCK_SIZE);
    auto any = [](char *, uint32_t) -> bool { return true; };
    {
        UserSession user(7, 7);
        file_info_t meta;
        ASSERT_EQ((int)__i_fs.getFileMeta("/dev/mmcblk0", meta), (int)PDI_OK);
        ASSERT_EQ((int)meta.m_perms, 0600);
        ASSERT_EQ((int)meta.m_size, (int)(SD_BLOCKS * SD_BLOCK_SIZE));
        ASSERT_EQ(__i_fs.readFile("/dev/mmcblk0", 64, any), (int)PDI_ERR_PERM);
        ASSERT_EQ(__i_fs.readFile("/dev/mmcblk0p1", 64, any), (int)PDI_ERR_PERM);
        ASSERT_EQ(__i_fs.editFile("/dev/mmcblk0", 0, "x", 1), (int)PDI_ERR_PERM);
        ASSERT_EQ(__i_fs.writeFile("/dev/mmcblk0p1", "x", 1), (int)PDI_ERR_PERM);
        ASSERT_TRUE(pditest::saw(pditest::Shell(7, 7).run("dd if=/dev/mmcblk0 of=/tmp/b.bin count=1"), "-12"));
    }
    ASSERT_EQ(0, memcmp(bench.m_card->data(), card.data(), card.size()));
    ASSERT_EQ(__i_fs.readFile("/dev/mmcblk0", 64, [](char *, uint32_t) -> bool { return false; }), 64);
}

TEST(driver_core, a_user_reads_a_mounted_card_but_cannot_change_or_remove_roots_files)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/root.txt", "owned by root", 13), 13);
    ASSERT_EQ((int)__i_fs.createDirectory("/mnt/rootdir"), (int)PDI_OK);
    {
        UserSession user(7, 7);
        ASSERT_STREQ(readAll("/mnt/root.txt").c_str(), "owned by root");
        ASSERT_EQ(__i_fs.writeFile("/mnt/root.txt", "mine", 4), (int)PDI_ERR_PERM);
        ASSERT_EQ(__i_fs.editFile("/mnt/root.txt", 0, "m", 1), (int)PDI_ERR_PERM);
        ASSERT_EQ((int)__i_fs.deleteFile("/mnt/root.txt"), (int)PDI_ERR_PERM);
        ASSERT_EQ((int)__i_fs.deleteDirectory("/mnt/rootdir"), (int)PDI_ERR_PERM);
        ASSERT_EQ(__i_fs.setFilePermissions("/mnt/root.txt", 0444), (int)PDI_ERR_PERM);
    }
    ASSERT_STREQ(readAll("/mnt/root.txt").c_str(), "owned by root");
    ASSERT_TRUE(__i_fs.isDirExist("/mnt/rootdir"));
}

TEST(driver_core, a_read_of_the_disk_into_a_pipe_stops_once_the_pipe_is_full)
{
    const char *const lines[] = {"hexdump /dev/mmcblk0 | head 2", "cat /dev/mmcblk0 | head 1"};
    for (const char *line : lines)
    {
        SdServiceBench bench;
        pditest::Shell sh;
        int before = bench.m_card->m_reads;

        sh.run(line);

        ASSERT_TRUE(bench.m_card->m_reads - before < 16);
    }
}

TEST(driver_core, ctrl_c_stops_a_whole_disk_read_part_way)
{
    const char *const lines[] = {"hexdump /dev/mmcblk0\r\x03", "dd if=/dev/mmcblk0 of=/dev/null\r\x03"};
    for (const char *keys : lines)
    {
        SdServiceBench bench;
        pditest::Shell sh;
        int before = bench.m_card->m_reads;

        std::string out = sh.type(keys);

        ASSERT_TRUE(pditest::saw(out, (std::string("CmdErr : ") + std::to_string((int)CMD_ERROR_CANCELED)).c_str()));
        ASSERT_TRUE(bench.m_card->m_reads - before < 16);
    }
}

TEST(driver_core, a_whole_disk_read_runs_to_its_end_with_no_key_pressed)
{
    SdServiceBench bench;
    pditest::Shell sh;
    int before = bench.m_card->m_reads;

    std::string out = sh.type("dd if=/dev/mmcblk0 of=/dev/null\r");

    ASSERT_TRUE(pditest::saw(out, "8192+0 records in"));
    ASSERT_EQ(bench.m_card->m_reads - before, (int)SD_BLOCKS);
}

TEST(driver_core, the_shell_mounts_and_unmounts_a_card_with_a_trailing_slash_on_the_directory)
{
    FatCardBench bench;
    pditest::Shell sh;

    ASSERT_TRUE(pditest::saw(sh.run("mount /dev/mmcblk0"), "-3602"));
    ASSERT_FALSE(pditest::saw(sh.run("mount /dev/mmcblk0 /mnt/"), "CmdErr"));
    ASSERT_TRUE(pditest::saw(sh.run("mount"), "vfat"));
    ASSERT_FALSE(pditest::saw(sh.run("umount /mnt/"), "CmdErr"));
    ASSERT_FALSE(pditest::saw(sh.run("mount"), "vfat"));
    ASSERT_TRUE(pditest::saw(sh.run("umount /mnt"), "umount: /mnt : -6"));
    ASSERT_TRUE(pditest::saw(sh.run("umount"), "-3602"));
}

TEST(driver_core, blkid_given_a_node_names_only_that_one)
{
    SdServiceBench bench;
    putPartitionTable(bench.m_card, 2048, 4096, 0x0C);
    putFatBootSector(bench.m_card, 2048);
    rebind(bench);
    pditest::Shell sh;

    std::string one = sh.run("blkid /dev/mmcblk0p1");
    ASSERT_TRUE(pditest::saw(one, "/dev/mmcblk0p1: TYPE=\"vfat\""));
    ASSERT_FALSE(pditest::saw(one, "PTTYPE"));
    ASSERT_FALSE(pditest::saw(sh.run("blkid /dev/null"), "TYPE"));
}

TEST(driver_core, dd_names_an_output_it_cannot_open_by_its_code)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/ro.txt", "keep", 4), 4);
    uint16_t perms = 0444;
    ASSERT_EQ(__i_fs.setFileAttr("/mnt/ro.txt", FILE_ATTR_PERMS, &perms, sizeof(perms)), (int)sizeof(perms));
    ASSERT_EQ(__i_fs.writeFile("/tmp/src.txt", "data", 4), 4);

    std::string out = pditest::Shell().run("dd if=/tmp/src.txt of=/mnt/ro.txt");

    ASSERT_TRUE(pditest::saw(out, "dd: /mnt/ro.txt : -4807"));
    ASSERT_STREQ(readAll("/mnt/ro.txt").c_str(), "keep");
    __i_fs.deleteFile("/tmp/src.txt");
}

TEST(driver_core, dd_names_a_disk_that_fails_a_read_or_a_write_by_its_code)
{
    SdServiceBench bench;
    pditest::Shell sh;
    bench.m_card->m_present = false;

    std::string in = sh.run("dd if=/dev/mmcblk0 of=/tmp/blk.bin count=1");
    ASSERT_TRUE(pditest::saw(in, "dd: /dev/mmcblk0 : -6"));
    std::string out = sh.run("dd if=/dev/zero of=/dev/mmcblk0 count=1");
    ASSERT_TRUE(pditest::saw(out, "dd: /dev/mmcblk0 : -6"));
    ASSERT_TRUE(pditest::saw(out, "0+0 records out"));
    __i_fs.deleteFile("/tmp/blk.bin");
}

TEST(driver_core, dd_refuses_more_operands_than_it_takes)
{
    pditest::Shell sh;

    std::string out = sh.run("dd if=/a of=/b bs=1 count=1 skip=0 seek=0 extra=1");

    ASSERT_TRUE(pditest::saw(out, "too many arguments"));
}

TEST(driver_core, udevadm_info_marks_a_device_left_out_of_the_scan)
{
    ServiceBench bench;
    __i_dvc_ctrl.useVirtualClock(true);
    bench.m_i2c.m_probe_ms = DEVICE_SCAN_PROBE_MAX_MS + 1;
    writeDevicesConf("i2c0 0x60 testchip\n");
    bench.m_mgr->trigger();
    bench.m_i2c.m_probe_ms = 0;
    __i_dvc_ctrl.useVirtualClock(false);

    std::string info = pditest::Shell().run("udevadm info");

    ASSERT_TRUE(pditest::saw(info, "absent manual"));
    bench.m_mgr->trigger();
    ASSERT_FALSE(pditest::saw(pditest::Shell().run("udevadm info"), "manual"));
}

TEST(driver_core, a_layout_read_that_fails_on_either_block_names_nothing_and_is_read_again_later)
{
    SdServiceBench bench;
    putFatBootSector(bench.m_card, 0);
    iBlockDriverInterface *card = boundCard(*bench.m_mgr);
    ASSERT_NOT_NULL(card);

    for (int which = 1; which <= 2; which++)
    {
        card->forgetLayout();
        bench.m_card->m_fail_read_at = bench.m_card->m_reads + which;
        ASSERT_EQ((int)card->scanLayout(), (int)PDI_ERR_IO);
        bench.m_card->m_fail_read_at = 0;
        ASSERT_EQ((int)card->fsType(), (int)BLOCK_FS_VFAT);
    }
}

TEST(driver_core, sys_block_lists_a_disk_and_each_partition_with_their_attributes)
{
    SdServiceBench bench;
    putPartitionTable(bench.m_card, 2048, 4096, 0x0C);
    rebind(bench);
    pditest::Shell sh;

    std::string disk = sh.run("ls /sys/block/mmcblk0");
    ASSERT_TRUE(pditest::saw(disk, "size"));
    ASSERT_TRUE(pditest::saw(disk, "mmcblk0p1"));
    std::string part = sh.run("ls /sys/block/mmcblk0/mmcblk0p1");
    ASSERT_TRUE(pditest::saw(part, "size"));
    ASSERT_TRUE(pditest::saw(part, "start"));
}

TEST(driver_core, the_fat_disk_glue_answers_each_question_fatfs_asks_of_a_mounted_card)
{
    MountedFatBench bench;
    ASSERT_EQ((int)bench.mounted, (int)PDI_OK);
    FatFsInterfaceImpl *vol = nullptr;
    BYTE drive = 0;
    for (; drive < FAT_VOLUMES_MAX && nullptr == vol; drive++)
    {
        vol = FatFsInterfaceImpl::byDrive(drive);
    }
    ASSERT_NOT_NULL(vol);
    drive--;
    LBA_t sectors = 0;
    WORD size = 0;
    DWORD erase = 0;
    BYTE block[SD_BLOCK_SIZE];

    ASSERT_EQ((int)disk_ioctl(drive, GET_SECTOR_COUNT, &sectors), (int)RES_OK);
    ASSERT_EQ((int)sectors, (int)SD_BLOCKS);
    ASSERT_EQ((int)disk_ioctl(drive, GET_SECTOR_SIZE, &size), (int)RES_OK);
    ASSERT_EQ((int)size, (int)SD_BLOCK_SIZE);
    ASSERT_EQ((int)disk_ioctl(drive, GET_BLOCK_SIZE, &erase), (int)RES_OK);
    ASSERT_EQ((int)erase, 1);
    ASSERT_EQ((int)disk_ioctl(drive, CTRL_SYNC, nullptr), (int)RES_OK);
    ASSERT_EQ((int)disk_ioctl(drive, 200, nullptr), (int)RES_PARERR);
    ASSERT_EQ((int)disk_initialize(drive), 0);
    ASSERT_EQ((int)disk_read(drive, block, SD_BLOCKS - 1, 2), (int)RES_PARERR);
    ASSERT_EQ((int)disk_write(drive, block, SD_BLOCKS, 1), (int)RES_PARERR);

    BYTE unused = (BYTE)((drive + 1) % FAT_VOLUMES_MAX);
    ASSERT_NULL(FatFsInterfaceImpl::byDrive(unused));
    ASSERT_EQ((int)disk_status(unused), (int)(STA_NOINIT | STA_NODISK));
    ASSERT_EQ((int)disk_read(unused, block, 0, 1), (int)RES_NOTRDY);
    ASSERT_EQ((int)disk_write(unused, block, 0, 1), (int)RES_NOTRDY);
    ASSERT_EQ((int)disk_ioctl(unused, CTRL_SYNC, nullptr), (int)RES_NOTRDY);

    bench.m_card->m_present = false;
    ASSERT_EQ((int)disk_write(drive, block, 0, 1), (int)RES_ERROR);
    ASSERT_EQ(__i_fs.writeFile("/mnt/x.txt", "x", 1), (int)PDI_ERR_NOT_FOUND);
    bench.m_card->m_present = true;
}

TEST(driver_core, a_card_bound_again_while_mounted_is_refused_until_it_is_mounted_again)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/before.txt", "before", 6), 6);
    std::vector<uint8_t> card(bench.m_card->data(), bench.m_card->data() + SD_BLOCKS * SD_BLOCK_SIZE);

    rebind(bench);

    ASSERT_EQ(__i_fs.readFile("/mnt/before.txt", 16, [](char *, uint32_t) -> bool { return true; }), (int)PDI_ERR_NOT_FOUND);
    ASSERT_EQ(__i_fs.writeFile("/mnt/after.txt", "after", 5), (int)PDI_ERR_NOT_FOUND);
    ASSERT_EQ(0, memcmp(bench.m_card->data(), card.data(), card.size()));

    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_OK);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);
    ASSERT_STREQ(readAll("/mnt/before.txt").c_str(), "before");
}

TEST(driver_core, each_block_driver_made_carries_a_serial_of_its_own)
{
    SdServiceBench bench;
    iBlockDriverInterface *first = boundCard(*bench.m_mgr);
    ASSERT_NOT_NULL(first);
    uint32_t before = first->serial();

    rebind(bench);

    iBlockDriverInterface *second = boundCard(*bench.m_mgr);
    ASSERT_NOT_NULL(second);
    ASSERT_NE(second->serial(), before);
}

TEST(driver_core, a_mounted_card_taken_out_and_put_back_answers_not_found_until_mounted_again)
{
    MountedFatBench bench;
    ASSERT_EQ(__i_fs.writeFile("/mnt/kept.txt", "kept", 4), 4);

    bench.m_card->m_present = false;
    bench.m_mgr->scan();
    ASSERT_EQ(__i_fs.readFile("/mnt/kept.txt", 16, [](char *, uint32_t) -> bool { return true; }), (int)PDI_ERR_NOT_FOUND);

    bench.m_card->reinsert();
    bench.m_mgr->scan();
    ASSERT_NOT_NULL(boundCard(*bench.m_mgr));
    ASSERT_EQ(__i_fs.readFile("/mnt/kept.txt", 16, [](char *, uint32_t) -> bool { return true; }), (int)PDI_ERR_NOT_FOUND);

    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_OK);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);
    ASSERT_STREQ(readAll("/mnt/kept.txt").c_str(), "kept");
}

TEST(driver_core, a_mount_past_the_volumes_fat_keeps_is_refused_as_full)
{
    SdServiceBench bench;
    fillCard(bench.m_card);
    memset(bench.m_card->data() + 446, 0, 64);
    const uint32_t starts[3] = {1024, 3072, 5120};
    for (uint8_t i = 0; i < 3; i++)
    {
        putPartitionEntry(bench.m_card, i, starts[i], 2048);
        putFatVolume(bench.m_card, starts[i], 2048);
    }
    rebind(bench);
    const char *dirs[3] = {"/m1", "/m2", "/m3"};
    for (uint8_t i = 0; i < 3; i++)
    {
        __i_fs.createDirectory(dirs[i]);
    }

    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0p1", "/m1"), (int)PDI_OK);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0p2", "/m2"), (int)PDI_OK);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0p3", "/m3"), (int)PDI_ERR_FULL);
    ASSERT_EQ(__i_fs.writeFile("/m2/two.txt", "two", 3), 3);
    ASSERT_FALSE(__i_fs.isFileExist("/m1/two.txt"));

    ASSERT_EQ((int)__i_fs.umount("/m1"), (int)PDI_OK);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0p3", "/m3"), (int)PDI_OK);
    ASSERT_EQ((int)__i_fs.umount("/m2"), (int)PDI_OK);
    ASSERT_EQ((int)__i_fs.umount("/m3"), (int)PDI_OK);
    for (uint8_t i = 0; i < 3; i++)
    {
        __i_fs.deleteDirectory(dirs[i]);
    }
}

/* ------------------------------------------- wipefs, sfdisk and mkfs */

namespace
{

/**
 * The card bench blank: every block zero, read afresh.
 */
struct BlankCardBench : SdServiceBench
{
    BlankCardBench()
    {
        memset(m_card->data(), 0, (size_t)m_card->blocks() * SD_BLOCK_SIZE);
        rebind(*this);
        __i_fs.createDirectory("/mnt");
    }

    ~BlankCardBench()
    {
        __i_fs.umount("/mnt");
        __i_fs.deleteDirectory("/mnt");
    }
};

uint32_t entryValue(SdCardSim *card, uint8_t slot, uint8_t at)
{
    uint32_t value = 0;
    memcpy(&value, card->data() + 446 + slot * 16 + at, 4);
    return value;
}

} // namespace

TEST(driver_core, wipefs_erases_a_whole_card_fat_the_way_linux_reports_it)
{
    FatCardBench bench;
    pditest::Shell sh;

    std::string out = sh.run("wipefs -a /dev/mmcblk0");

    ASSERT_TRUE(pditest::saw(out, "/dev/mmcblk0: 8 bytes were erased at offset 0x00000036 (vfat): 46 41 54 31 32 20 20 20"));
    ASSERT_TRUE(pditest::saw(out, "/dev/mmcblk0: 2 bytes were erased at offset 0x000001fe (vfat): 55 aa"));
    ASSERT_FALSE(pditest::saw(sh.run("blkid"), "TYPE="));
    ASSERT_EQ((int)bench.m_card->data()[0], 0xEB);
}

TEST(driver_core, wipefs_without_a_lists_the_signatures_and_erases_nothing)
{
    FatCardBench bench;
    pditest::Shell sh;

    std::string out = sh.run("wipefs /dev/mmcblk0");

    ASSERT_TRUE(pditest::saw(out, "DEVICE"));
    ASSERT_TRUE(pditest::saw(out, "OFFSET"));
    ASSERT_TRUE(pditest::saw(out, "0x36"));
    ASSERT_TRUE(pditest::saw(out, "0x1fe"));
    ASSERT_TRUE(pditest::saw(out, "vfat"));
    ASSERT_FALSE(pditest::saw(out, "erased"));
    ASSERT_EQ((int)bench.m_card->data()[510], 0x55);
    ASSERT_TRUE(pditest::saw(pditest::Shell(7, 7).run("wipefs /dev/mmcblk0"), "wipefs: /dev/mmcblk0 : -12"));
}

TEST(driver_core, wipefs_takes_the_partition_table_away_and_leaves_the_data)
{
    BlankCardBench bench;
    putPartitionTable(bench.m_card, 2048, 4096, 0x0C);
    putFatVolume(bench.m_card, 2048, 4096);
    rebind(bench);
    pditest::Shell sh;
    ASSERT_TRUE(pditest::saw(sh.run("lsblk"), "mmcblk0p1"));

    std::string out = sh.run("wipefs -a /dev/mmcblk0");

    ASSERT_TRUE(pditest::saw(out, "2 bytes were erased at offset 0x000001fe (dos): 55 aa"));
    ASSERT_FALSE(pditest::saw(sh.run("lsblk"), "mmcblk0p1"));
    ASSERT_EQ((int)bench.m_card->data()[510], 0);
    ASSERT_EQ((int)bench.m_card->data()[2048 * SD_BLOCK_SIZE + 54], 'F');
}

TEST(driver_core, wipefs_of_a_blank_card_says_nothing_and_refuses_what_it_must)
{
    BlankCardBench bench;
    pditest::Shell sh;
    std::string blank = sh.run("wipefs -a /dev/mmcblk0");
    ASSERT_FALSE(pditest::saw(blank, "erased"));
    ASSERT_FALSE(pditest::saw(blank, "CmdErr"));

    ASSERT_TRUE(pditest::saw(sh.run("wipefs /dev/null"), "wipefs: /dev/null : -6"));
    ASSERT_TRUE(pditest::saw(pditest::Shell(7, 7).run("wipefs -a /dev/mmcblk0"), "wipefs: /dev/mmcblk0 : -12"));

    putFatVolume(bench.m_card, 0, SD_BLOCKS);
    rebind(bench);
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);
    ASSERT_TRUE(pditest::saw(sh.run("wipefs -a /dev/mmcblk0"), "wipefs: /dev/mmcblk0 : -10"));
    ASSERT_EQ((int)bench.m_card->data()[510], 0x55);
}

TEST(driver_core, sfdisk_makes_a_partition_from_a_piped_line)
{
    BlankCardBench bench;
    pditest::Shell sh;

    std::string out = sh.run("echo \",,c\" | sfdisk /dev/mmcblk0");

    ASSERT_TRUE(pditest::saw(out, "Created a new DOS disklabel with disk identifier 0x"));
    ASSERT_TRUE(pditest::saw(out, "/dev/mmcblk0p1: Created a new partition 1 of type 'W95 FAT32 (LBA)' and of size 3 MiB."));
    ASSERT_TRUE(pditest::saw(out, "The partition table has been altered."));
    ASSERT_EQ(entryValue(bench.m_card, 0, 8), 2048u);
    ASSERT_EQ(entryValue(bench.m_card, 0, 12), SD_BLOCKS - 2048u);
    ASSERT_EQ((int)bench.m_card->data()[446 + 4], 0x0C);
    ASSERT_TRUE(pditest::saw(sh.run("lsblk"), "mmcblk0p1"));
}

TEST(driver_core, sfdisk_takes_several_partitions_from_a_file_and_keeps_the_identifier)
{
    BlankCardBench bench;
    pditest::Shell sh;
    __i_fs.writeFile("/w_layout.txt", ",1M,c,*\n# a comment\n\n,,83\n", 26, false);

    std::string first = sh.run("sfdisk /dev/mmcblk0 < /w_layout.txt");
    uint32_t id = 0;
    memcpy(&id, bench.m_card->data() + 440, 4);
    std::string again = sh.run("sfdisk /dev/mmcblk0 < /w_layout.txt");
    uint32_t idagain = 0;
    memcpy(&idagain, bench.m_card->data() + 440, 4);
    __i_fs.deleteFile("/w_layout.txt");

    ASSERT_TRUE(pditest::saw(first, "Created a new DOS disklabel"));
    ASSERT_FALSE(pditest::saw(again, "Created a new DOS disklabel"));
    ASSERT_NE(id, 0u);
    ASSERT_EQ(id, idagain);
    ASSERT_EQ(entryValue(bench.m_card, 0, 8), 2048u);
    ASSERT_EQ(entryValue(bench.m_card, 0, 12), 2048u);
    ASSERT_EQ((int)bench.m_card->data()[446], 0x80);
    ASSERT_EQ(entryValue(bench.m_card, 1, 8), 4096u);
    ASSERT_EQ(entryValue(bench.m_card, 1, 12), SD_BLOCKS - 4096u);
    ASSERT_EQ((int)bench.m_card->data()[446 + 16 + 4], 0x83);
}

TEST(driver_core, sfdisk_l_lists_the_table_the_way_fdisk_does)
{
    BlankCardBench bench;
    pditest::Shell sh;
    sh.run("echo \",,c\" | sfdisk /dev/mmcblk0");

    std::string out = sh.run("sfdisk -l /dev/mmcblk0");

    ASSERT_TRUE(pditest::saw(out, "Disk /dev/mmcblk0: 4 MiB, 4194304 bytes, 8192 sectors"));
    ASSERT_TRUE(pditest::saw(out, "Units: sectors of 1 * 512 = 512 bytes"));
    ASSERT_TRUE(pditest::saw(out, "Disklabel type: dos"));
    ASSERT_TRUE(pditest::saw(out, "Disk identifier: 0x"));
    ASSERT_TRUE(pditest::saw(out, "Device"));
    ASSERT_TRUE(pditest::saw(out, "/dev/mmcblk0p1"));
    ASSERT_TRUE(pditest::saw(out, "2048"));
    ASSERT_TRUE(pditest::saw(out, "8191"));
    ASSERT_TRUE(pditest::saw(out, "6144"));
    ASSERT_TRUE(pditest::saw(out, "W95 FAT32 (LBA)"));

    std::string all = sh.run("sfdisk -l");
    ASSERT_TRUE(pditest::saw(all, "Disk /dev/mmcblk0:"));
}

TEST(driver_core, sfdisk_delete_keeps_the_numbers_of_the_partitions_left)
{
    BlankCardBench bench;
    pditest::Shell sh;
    __i_fs.writeFile("/w_two.txt", ",1M,c\n,,83\n", 11, false);
    sh.run("sfdisk /dev/mmcblk0 < /w_two.txt");
    __i_fs.deleteFile("/w_two.txt");

    std::string one = sh.run("sfdisk --delete /dev/mmcblk0 1");
    std::string list = sh.run("lsblk");
    ASSERT_TRUE(pditest::saw(one, "The partition table has been altered."));
    ASSERT_FALSE(pditest::saw(list, "mmcblk0p1"));
    ASSERT_TRUE(pditest::saw(list, "mmcblk0p2"));
    ASSERT_TRUE(pditest::saw(sh.run("sfdisk --delete /dev/mmcblk0 1"), "sfdisk: /dev/mmcblk0 : -6"));

    sh.run("sfdisk --delete /dev/mmcblk0");
    ASSERT_FALSE(pditest::saw(sh.run("lsblk"), "mmcblk0p"));
    ASSERT_TRUE(pditest::saw(sh.run("blkid"), "PTTYPE=\"dos\""));
}

TEST(driver_core, sfdisk_writes_nothing_when_a_line_or_the_disk_is_wrong)
{
    BlankCardBench bench;
    pditest::Shell sh;

    ASSERT_TRUE(pditest::saw(sh.run("echo \",9G,c\" | sfdisk /dev/mmcblk0"), "sfdisk: line 1 : -18"));
    __i_fs.writeFile("/w_bad.txt", ",1M,c\n,,zz\n", 11, false);
    ASSERT_TRUE(pditest::saw(sh.run("sfdisk /dev/mmcblk0 < /w_bad.txt"), "sfdisk: line 2 : -2"));
    __i_fs.writeFile("/w_bad.txt", "2048,64\n2112,64\n2176,64\n2240,64\n2304,64\n", 40, false);
    ASSERT_TRUE(pditest::saw(sh.run("sfdisk /dev/mmcblk0 < /w_bad.txt"), "sfdisk: line 5 : -25"));
    __i_fs.deleteFile("/w_bad.txt");
    ASSERT_EQ((int)bench.m_card->data()[510], 0);

    ASSERT_TRUE(pditest::saw(sh.run("sfdisk /dev/mmcblk0"), "sfdisk: no partitions given"));
    ASSERT_TRUE(pditest::saw(pditest::Shell(7, 7).run("echo \",,c\" | sfdisk /dev/mmcblk0"), "sfdisk: /dev/mmcblk0 : -12"));
    sh.run("echo \",,c\" | sfdisk /dev/mmcblk0");
    ASSERT_TRUE(pditest::saw(sh.run("echo \",,c\" | sfdisk /dev/mmcblk0p1"), "sfdisk: /dev/mmcblk0p1 : -2"));
}

TEST(driver_core, mkfs_vfat_formats_a_partition_that_then_mounts_and_holds_files)
{
    BlankCardBench bench;
    pditest::Shell sh;
    sh.run("echo \",,c\" | sfdisk /dev/mmcblk0");

    std::string out = sh.run("mkfs -t vfat /dev/mmcblk0p1");

    ASSERT_TRUE(pditest::saw(out, "this may take some time"));
    ASSERT_FALSE(pditest::saw(out, "CmdErr"));
    ASSERT_TRUE(pditest::saw(sh.run("blkid"), "/dev/mmcblk0p1: TYPE=\"vfat\""));
    ASSERT_EQ((int)bench.m_card->data()[2048 * SD_BLOCK_SIZE + 16], 2);
    ASSERT_EQ((int)bench.m_card->data()[446 + 4], 0x0C);
    ASSERT_FALSE(pditest::saw(sh.run("mount /dev/mmcblk0p1 /mnt"), "mount:"));
    sh.run("echo on a fresh card > /mnt/new.txt");
    ASSERT_TRUE(pditest::saw(sh.run("cat /mnt/new.txt"), "on a fresh card"));
}

TEST(driver_core, mkfs_vfat_formats_a_whole_card_with_no_table)
{
    BlankCardBench bench;
    pditest::Shell sh;
    sh.run("echo \",,c\" | sfdisk /dev/mmcblk0");
    sh.run("wipefs -a /dev/mmcblk0");

    sh.run("mkfs -t vfat /dev/mmcblk0");

    std::string ids = sh.run("blkid");
    ASSERT_TRUE(pditest::saw(ids, "/dev/mmcblk0: TYPE=\"vfat\""));
    ASSERT_FALSE(pditest::saw(ids, "PTTYPE"));
    ASSERT_FALSE(pditest::saw(sh.run("mount /dev/mmcblk0 /mnt"), "mount:"));
}

TEST(driver_core, mkfs_refuses_a_type_it_cannot_make_a_mounted_disk_and_a_user)
{
    BlankCardBench bench;
    pditest::Shell sh;

    ASSERT_TRUE(pditest::saw(sh.run("mkfs -t ext4 /dev/mmcblk0"), "mkfs: /dev/mmcblk0 : -7"));
    ASSERT_TRUE(pditest::saw(sh.run("mkfs -t vfat /dev/null"), "mkfs: /dev/null : -6"));
    ASSERT_TRUE(pditest::saw(sh.run("mkfs /dev/mmcblk0"), "CmdErr"));
    ASSERT_TRUE(pditest::saw(pditest::Shell(7, 7).run("mkfs -t vfat /dev/mmcblk0"), "mkfs: /dev/mmcblk0 : -12"));
    sh.run("echo \",,c\" | sfdisk /dev/mmcblk0");
    std::string partitioned = sh.run("mkfs -t vfat /dev/mmcblk0");
    ASSERT_TRUE(pditest::saw(partitioned, "mkfs: /dev/mmcblk0 : -13"));
    ASSERT_FALSE(pditest::saw(partitioned, "this may take some time"));
    sh.run("wipefs -a /dev/mmcblk0");
    sh.run("mkfs -t vfat /dev/mmcblk0");
    ASSERT_EQ((int)FatFsInterfaceImpl::mountDisk("/dev/mmcblk0", "/mnt"), (int)PDI_OK);
    ASSERT_TRUE(pditest::saw(sh.run("mkfs -t vfat /dev/mmcblk0"), "mkfs: /dev/mmcblk0 : -10"));
}

TEST(driver_core, mkfs_littlefs_makes_a_partition_that_mounts_and_keeps_its_files)
{
    BlankCardBench bench;
    pditest::Shell sh;
    sh.run("echo \",,83\" | sfdisk /dev/mmcblk0");

    std::string out = sh.run("mkfs -t littlefs /dev/mmcblk0p1");

    ASSERT_TRUE(pditest::saw(out, "this may take some time"));
    ASSERT_FALSE(pditest::saw(out, "CmdErr"));
    ASSERT_TRUE(pditest::saw(sh.run("blkid"), "/dev/mmcblk0p1: TYPE=\"littlefs\""));
    ASSERT_FALSE(pditest::saw(sh.run("mount /dev/mmcblk0p1 /mnt"), "mount:"));
    ASSERT_TRUE(pditest::saw(sh.run("mount"), "littlefs  mmcblk0p1"));
    sh.run("mkdir /mnt/logs");
    sh.run("echo kept on the card > /mnt/logs/a.txt");
    sh.run("chmod 600 /mnt/logs/a.txt");
    ASSERT_FALSE(pditest::saw(sh.run("umount /mnt"), "umount:"));
    ASSERT_FALSE(__i_fs.isFileExist("/mnt/logs/a.txt"));

    ASSERT_FALSE(pditest::saw(sh.run("mount /dev/mmcblk0p1 /mnt"), "mount:"));
    ASSERT_TRUE(pditest::saw(sh.run("cat /mnt/logs/a.txt"), "kept on the card"));
    ASSERT_TRUE(pditest::saw(sh.run("ls /mnt/logs"), "-rw-------"));
}

TEST(driver_core, mkfs_littlefs_makes_a_whole_card_and_wipefs_takes_it_away)
{
    BlankCardBench bench;
    pditest::Shell sh;

    sh.run("mkfs -t littlefs /dev/mmcblk0");
    ASSERT_TRUE(pditest::saw(sh.run("blkid"), "/dev/mmcblk0: TYPE=\"littlefs\""));
    ASSERT_TRUE(pditest::saw(sh.run("wipefs /dev/mmcblk0"), "littlefs"));

    std::string out = sh.run("wipefs -a /dev/mmcblk0");

    ASSERT_TRUE(pditest::saw(out, "8 bytes were erased at offset 0x00000008 (littlefs): 6c 69 74 74 6c 65 66 73"));
    ASSERT_TRUE(pditest::saw(out, "8 bytes were erased at offset 0x00001008 (littlefs)"));
    ASSERT_FALSE(pditest::saw(sh.run("blkid"), "TYPE="));
    ASSERT_TRUE(pditest::saw(sh.run("mount /dev/mmcblk0 /mnt"), "mount: /dev/mmcblk0 : "));
}

TEST(driver_core, a_mounted_littlefs_disk_is_busy_for_mkfs_wipefs_and_an_open_file)
{
    BlankCardBench bench;
    pditest::Shell sh;
    sh.run("mkfs -t littlefs /dev/mmcblk0");
    sh.run("mount /dev/mmcblk0 /mnt");

    ASSERT_TRUE(pditest::saw(sh.run("mkfs -t littlefs /dev/mmcblk0"), "mkfs: /dev/mmcblk0 : -10"));
    ASSERT_TRUE(pditest::saw(sh.run("wipefs -a /dev/mmcblk0"), "wipefs: /dev/mmcblk0 : -10"));
    ASSERT_TRUE(pditest::saw(sh.run("mount /dev/mmcblk0 /mnt"), "mount: /dev/mmcblk0 : -10"));
    pdi_fhandle_t handle = __i_fs.openFile("/mnt/open.txt", FILE_OPEN_WRITE | FILE_OPEN_CREATE);
    ASSERT_TRUE(handle >= 0);
    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_ERR_BUSY);
    __i_fs.closeFile(handle);
    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_OK);
}

TEST(driver_core, a_littlefs_disk_taken_away_answers_with_an_error)
{
    BlankCardBench bench;
    pditest::Shell sh;
    sh.run("mkfs -t littlefs /dev/mmcblk0");
    sh.run("mount /dev/mmcblk0 /mnt");
    bench.m_card->m_present = false;

    int written = __i_fs.writeFile("/mnt/after.txt", "after", 5);

    bench.m_card->m_present = true;
    ASSERT_TRUE(written < 0);
    ASSERT_EQ((int)__i_fs.umount("/mnt"), (int)PDI_OK);
}

namespace
{

/**
 * An /etc/fstab holding the lines given for as long as it lives.
 */
struct FstabFile
{
    explicit FstabFile(const char *body)
    {
        __i_fs.writeFile(FSTAB_PATH, body, (uint32_t)strlen(body), false);
    }

    ~FstabFile()
    {
        __i_fs.deleteFile(FSTAB_PATH);
    }
};

} // namespace

TEST(driver_core, fstab_mounts_a_listed_partition_when_its_card_comes)
{
    BlankCardBench bench;
    pditest::Shell sh;
    sh.run("echo \",,c\" | sfdisk /dev/mmcblk0");
    sh.run("mkfs -t vfat /dev/mmcblk0p1");
    FstabFile fstab("# cards\n/dev/mmcblk0p1  /mnt/  vfat  defaults  0  0\n");

    rebind(bench);

    std::string mounts = sh.run("mount");
    ASSERT_TRUE(pditest::saw(mounts, "/mnt"));
    ASSERT_TRUE(pditest::saw(mounts, "vfat      mmcblk0p1"));
}

TEST(driver_core, mount_a_mounts_what_fstab_lists_and_skips_noauto_and_what_is_mounted)
{
    BlankCardBench bench;
    pditest::Shell sh;
    __i_fs.writeFile("/w_two.txt", ",1M,83\n,,c\n", 12, false);
    sh.run("sfdisk /dev/mmcblk0 < /w_two.txt");
    __i_fs.deleteFile("/w_two.txt");
    sh.run("mkfs -t littlefs /dev/mmcblk0p1");
    sh.run("mkfs -t vfat /dev/mmcblk0p2");
    __i_fs.createDirectory("/mnt2");
    FstabFile fstab("/dev/mmcblk0p1 /mnt littlefs rw 0 0\n/dev/mmcblk0p2 /mnt2 vfat noauto,rw 0 0\n");

    std::string out = sh.run("mount -a");
    std::string again = sh.run("mount -a");

    std::string mounts = sh.run("mount");
    __i_fs.umount("/mnt2");
    __i_fs.deleteDirectory("/mnt2");
    ASSERT_FALSE(pditest::saw(out, "mount:"));
    ASSERT_FALSE(pditest::saw(again, "mount:"));
    ASSERT_FALSE(pditest::saw(again, "CmdErr"));
    ASSERT_TRUE(pditest::saw(mounts, "littlefs  mmcblk0p1"));
    ASSERT_FALSE(pditest::saw(mounts, "mmcblk0p2"));
}

TEST(driver_core, mount_a_names_a_failing_line_unless_it_is_nofail)
{
    BlankCardBench bench;
    pditest::Shell sh;
    sh.run("echo \",,c\" | sfdisk /dev/mmcblk0");
    sh.run("mkfs -t vfat /dev/mmcblk0p1");

    {
        FstabFile fstab("/dev/mmcblk0p3 /mnt auto defaults 0 0\n");
        ASSERT_TRUE(pditest::saw(sh.run("mount -a"), "mount: /dev/mmcblk0p3 : -6"));
    }
    {
        FstabFile fstab("/dev/mmcblk0p3 /mnt auto defaults,nofail 0 0\n");
        std::string out = sh.run("mount -a");
        ASSERT_FALSE(pditest::saw(out, "mount:"));
        ASSERT_FALSE(pditest::saw(out, "CmdErr"));
    }
    {
        FstabFile fstab("/dev/mmcblk0p1 /mnt littlefs defaults 0 0\n");
        ASSERT_TRUE(pditest::saw(sh.run("mount -a"), "mount: /dev/mmcblk0p1 : -7"));
        ASSERT_TRUE(pditest::saw(pditest::Shell(7, 7).run("mount -a"), "mount: /etc/fstab : -12"));
    }
    ASSERT_FALSE(pditest::saw(sh.run("mount"), "mmcblk0p1"));
}

TEST(driver_core, mount_a_tries_a_line_whose_directory_holds_another_device)
{
    BlankCardBench bench;
    pditest::Shell sh;
    __i_fs.writeFile("/w_two.txt", ",1M,c\n,,c\n", 11, false);
    sh.run("sfdisk /dev/mmcblk0 < /w_two.txt");
    __i_fs.deleteFile("/w_two.txt");
    sh.run("mkfs -t vfat /dev/mmcblk0p1");
    sh.run("mkfs -t vfat /dev/mmcblk0p2");
    sh.run("mount /dev/mmcblk0p2 /mnt");
    FstabFile fstab("/dev/mmcblk0p1 /mnt vfat defaults 0 0\n");

    std::string out = sh.run("mount -a");

    ASSERT_TRUE(pditest::saw(out, "mount: /dev/mmcblk0p1 : -13"));
    ASSERT_TRUE(pditest::saw(sh.run("mount"), "mmcblk0p2"));
}

TEST(driver_core, a_card_read_as_all_zeros_is_read_again_on_the_next_look)
{
    BlankCardBench bench;
    pditest::Shell sh;
    ASSERT_FALSE(pditest::saw(sh.run("lsblk"), "mmcblk0p1"));

    putPartitionTable(bench.m_card, 2048, 4096, 0x0C);

    ASSERT_TRUE(pditest::saw(sh.run("lsblk"), "mmcblk0p1"));
}

TEST(driver_core, sys_block_lists_each_partition_of_a_card_by_its_own_name)
{
    BlankCardBench bench;
    pditest::Shell sh;
    __i_fs.writeFile("/w_two.txt", ",1M,c\n,,83\n", 11, false);
    sh.run("sfdisk /dev/mmcblk0 < /w_two.txt");
    __i_fs.deleteFile("/w_two.txt");

    std::string listed = sh.run("ls /sys/block/mmcblk0");

    ASSERT_TRUE(pditest::saw(listed, "mmcblk0p1"));
    ASSERT_TRUE(pditest::saw(listed, "mmcblk0p2"));
    ASSERT_FALSE(pditest::saw(listed, "p1p2"));
    ASSERT_TRUE(pditest::saw(sh.run("cat /sys/block/mmcblk0/mmcblk0p2/start"), "4096"));
}

TEST(driver_core, free_space_and_staging_follow_the_filesystem_a_path_is_on)
{
    BlankCardBench bench;
    pditest::Shell sh;
    sh.run("mkfs -t vfat /dev/mmcblk0");
    sh.run("mount /dev/mmcblk0 /mnt");
    const vfs_mount_t *card = __i_fs.findMountForPath("/mnt");
    const vfs_mount_t *tmp = __i_fs.findMountForPath("/tmp");
    ASSERT_NOT_NULL(card);
    ASSERT_NOT_NULL(tmp);

    ASSERT_EQ(__i_fs.getFreeSize("/mnt/sub/x.wav"), card->m_backend->getFreeSize());
    ASSERT_EQ(__i_fs.getFreeSize("/mnt"), card->m_backend->getFreeSize());
    ASSERT_EQ(__i_fs.getFreeSize("/tmp/x"), tmp->m_backend->getFreeSize());
    ASSERT_EQ(__i_fs.getFreeSize("/etc/x"), __i_fs.getFreeSize());
    ASSERT_NE(__i_fs.getFreeSize("/mnt/x"), __i_fs.getFreeSize());

    ASSERT_EQ(std::string(__i_fs.getStagingDirectory("/mnt/sub/").c_str()), std::string("/mnt/.upload/"));
    ASSERT_EQ(std::string(__i_fs.getStagingDirectory("/mnt").c_str()), std::string("/mnt/.upload/"));
    ASSERT_EQ(std::string(__i_fs.getStagingDirectory("/etc/").c_str()), std::string(__i_fs.getTempDirectory()));
    ASSERT_EQ(std::string(__i_fs.getStagingDirectory("/tmp/").c_str()), std::string(__i_fs.getTempDirectory()));
    ASSERT_EQ(std::string(__i_fs.getStagingDirectory(nullptr).c_str()), std::string(__i_fs.getTempDirectory()));
}

#ifdef ENABLE_I2S_BUS
TEST(driver_core, arecord_onto_a_card_weighs_the_cards_free_space_not_the_roots)
{
    BlankCardBench bench;
    pditest::Shell sh;
    sh.run("mkfs -t vfat /dev/mmcblk0");
    writeDevicesConf("spi0 cs=12 mmc_spi\ni2s0 rx inmp441 sck=3 ws=4 sd=5 rate=8000\n");
    bench.m_mgr->trigger();
    sh.run("mount /dev/mmcblk0 /mnt");
    uint64_t rootfree = __i_fs.getFreeSize();
    uint64_t cardfree = __i_fs.getFreeSize("/mnt/v.wav");
    uint64_t fits = 100 * 16000;
    ASSERT_GT(cardfree, fits);
    ASSERT_LT(rootfree, fits);

    std::string started = sh.run("arecord -d 100 /mnt/v.wav");
    uint32_t began = __i_dvc_ctrl.millis_now();
    while ((__i_dvc_ctrl.millis_now() - began) < 50)
    {
        __task_scheduler.handle_tasks();
    }
    const uint8_t ctrl_c = 0x03;
    sh.type(&ctrl_c, 1);
    std::string toobig = sh.run("arecord -d 300 /mnt/w.wav");

    ASSERT_FALSE(pditest::saw(started, "not enough space"));
    ASSERT_TRUE(__i_fs.isFileExist("/mnt/v.wav"));
    ASSERT_GT((int)__i_fs.getFileSize("/mnt/v.wav"), 44);
    ASSERT_TRUE(pditest::saw(toobig, "not enough space"));
    ASSERT_FALSE(__i_fs.isFileExist("/mnt/w.wav"));
    __i_fs.deleteFile("/mnt/v.wav");
}
#endif
#endif

TEST(driver_core, a_card_line_holds_the_bus_pins_and_its_chip_select)
{
    SdServiceBench bench;
    bench.m_spi.m_defaults = {9, 10, 11};
    bench.m_mgr->trigger();

    for (gpio_id_t pin = 9; pin <= 12; pin++)
    {
        ASSERT_TRUE(__i_dvc_ctrl.isExceptionalGpio(pin));
    }
}

TEST(driver_core, a_card_whose_chip_select_is_a_bus_pin_is_refused_as_busy)
{
    SdServiceBench bench;
    bench.m_spi.m_defaults = {9, 10, 11};
    bench.m_spi.m_cards[10] = bench.m_card;
    writeDevicesConf("spi0 cs=10 mmc_spi\n");
    bench.m_mgr->trigger();

    const driver_device_t *dev = bench.find(SERIAL_IFACE_SPI, 10);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_FAILED);
    ASSERT_EQ((int)dev->m_result, (int)PDI_ERR_BUSY);
    bench.m_spi.m_cards.erase(10);
}

TEST(driver_core, an_spi_bus_line_moves_the_bus_and_the_pins_it_holds)
{
    SdServiceBench bench;
    bench.m_spi.m_defaults = {9, 10, 11};
    writeDevicesConf("spi0 bus spi sck=13 miso=14 mosi=15 clock=8000000\nspi0 cs=12 mmc_spi\n");
    bench.m_mgr->trigger();

    ASSERT_EQ((int)bench.m_mgr->busResult(SERIAL_IFACE_SPI), (int)PDI_OK);
    ASSERT_EQ((int)bench.m_spi.sck(), 13);
    ASSERT_EQ((int)bench.m_spi.miso(), 14);
    ASSERT_EQ((int)bench.m_spi.mosi(), 15);
    ASSERT_EQ((int)bench.m_spi.clock(), 8000000);
    ASSERT_TRUE(__i_dvc_ctrl.isExceptionalGpio(13));
    ASSERT_TRUE(__i_dvc_ctrl.isExceptionalGpio(15));
    ASSERT_FALSE(__i_dvc_ctrl.isExceptionalGpio(9));
    ASSERT_NOT_NULL(boundCard(*bench.m_mgr));
}

TEST(driver_core, an_spi_bus_line_missing_a_pin_is_refused_and_the_bus_keeps_its_defaults)
{
    SdServiceBench bench;
    bench.m_spi.m_defaults = {9, 10, 11};
    writeDevicesConf("spi0 bus spi sck=13 miso=14\nspi0 cs=12 mmc_spi\n");
    bench.m_mgr->trigger();

    ASSERT_EQ((int)bench.m_mgr->busResult(SERIAL_IFACE_SPI), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ((int)bench.m_spi.sck(), 9);
    ASSERT_TRUE(__i_dvc_ctrl.isExceptionalGpio(9));
}
#endif

#if defined(ENABLE_I2S_BUS) && defined(ENABLE_I2C_BUS)
namespace
{

/**
 * The serial service's bench with buses that keep default pins, the way a
 * port's do: SDA 4 and SCL 5 on i2c0, SCK 6, WS 7 and SD 8 on i2s0.
 */
struct PinBench : ServiceBench
{
    PinBench()
    {
        m_i2c.m_defaults = {4, 5};
        m_i2s.m_defaults = {6, 7, 8};
    }

    void declare(const char *body)
    {
        writeDevicesConf(body);
        if (nullptr != m_mgr)
        {
            m_mgr->trigger();
        }
    }

    bool held(gpio_id_t pin) { return __i_dvc_ctrl.isExceptionalGpio(pin); }

    int busResult(serial_iface_t bus) { return (nullptr == m_mgr) ? PDI_ERR_NOT_FOUND : (int)m_mgr->busResult(bus); }
};

} // namespace

TEST(driver_core, a_bus_default_pin_stays_a_gpio_until_a_line_uses_the_bus)
{
    PinBench bench;
    bench.declare("");

    for (gpio_id_t pin = 4; pin <= 8; pin++)
    {
        ASSERT_FALSE(bench.held(pin));
    }
    ASSERT_TRUE(__i_fs.isDirExist("/sys/class/gpio/4"));
}

TEST(driver_core, a_declared_mic_holds_the_pins_its_line_names)
{
    PinBench bench;
    bench.declare("i2s0 rx inmp441 sck=9 ws=10 sd=11\n");

    ASSERT_TRUE(bench.held(9));
    ASSERT_TRUE(bench.held(10));
    ASSERT_TRUE(bench.held(11));
    ASSERT_FALSE(bench.held(6));
}

TEST(driver_core, a_mic_line_without_pins_holds_and_runs_on_the_bus_default_pins)
{
    PinBench bench;
    bench.declare("i2s0 rx inmp441\n");

    ASSERT_TRUE(bench.held(6));
    ASSERT_TRUE(bench.held(7));
    ASSERT_TRUE(bench.held(8));
    const driver_device_t *dev = bench.find(SERIAL_IFACE_I2S, 0);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_EQ((int)bench.m_i2s.m_config.m_bclk, 6);
    ASSERT_EQ((int)bench.m_i2s.m_config.m_ws, 7);
    ASSERT_EQ((int)bench.m_i2s.m_config.m_din, 8);
}

TEST(driver_core, an_i2c_device_line_holds_the_bus_default_pins)
{
    PinBench bench;
    bench.declare("i2c0 0x50 testchip\n");

    ASSERT_TRUE(bench.held(4));
    ASSERT_TRUE(bench.held(5));
    ASSERT_FALSE(__i_fs.isDirExist("/sys/class/gpio/4"));
}

TEST(driver_core, a_bus_line_moves_the_bus_and_the_pins_it_holds)
{
    PinBench bench;
    bench.declare("i2c0 bus i2c sda=12 scl=13 clock=400000\ni2c0 0x50 testchip\n");

    ASSERT_EQ(bench.busResult(SERIAL_IFACE_I2C), (int)PDI_OK);
    ASSERT_TRUE(bench.held(12));
    ASSERT_TRUE(bench.held(13));
    ASSERT_FALSE(bench.held(4));
    ASSERT_FALSE(bench.held(5));
    ASSERT_EQ((int)bench.m_i2c.sda(), 12);
    ASSERT_EQ((int)bench.m_i2c.scl(), 13);
    ASSERT_EQ((int)bench.m_i2c.clock(), 400000);
}

TEST(driver_core, a_bus_line_is_not_listed_as_a_device)
{
    PinBench bench;
    bench.declare("i2c0 bus i2c sda=12 scl=13\n");
    pditest::Shell sh;

    ASSERT_NULL(bench.find(SERIAL_IFACE_I2C, DEVICE_TARGET_BUS));
    ASSERT_FALSE(pditest::saw(sh.run("udevadm info"), "i2c0"));
}

TEST(driver_core, a_bus_with_pins_is_looked_on_only_once_a_line_uses_it)
{
    PinBench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    bench.declare("");
    bench.scan();
    ASSERT_NULL(bench.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS));

    bench.declare("i2c0 bus i2c\n");

    ASSERT_NOT_NULL(bench.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS));
    ASSERT_TRUE(bench.held(4));
    ASSERT_TRUE(bench.held(5));
}

TEST(driver_core, taking_a_line_out_hands_its_pins_back_to_the_gpio_service)
{
    PinBench bench;
    bench.declare("i2s0 rx inmp441\ni2c0 0x50 testchip\n");
    ASSERT_TRUE(bench.held(4));
    ASSERT_TRUE(bench.held(6));
    ASSERT_FALSE(__i_fs.isDirExist("/sys/class/gpio/6"));

    bench.declare("");

    ASSERT_FALSE(bench.held(4));
    ASSERT_FALSE(bench.held(6));
    ASSERT_TRUE(__i_fs.isDirExist("/sys/class/gpio/6"));
}

TEST(driver_core, a_bus_line_with_only_one_pin_is_refused_and_the_bus_keeps_its_defaults)
{
    PinBench bench;
    bench.declare("i2c0 bus i2c sda=12\ni2c0 0x50 testchip\n");

    ASSERT_EQ(bench.busResult(SERIAL_IFACE_I2C), (int)PDI_ERR_INVALID_ARG);
    ASSERT_TRUE(bench.held(4));
    ASSERT_TRUE(bench.held(5));
    ASSERT_FALSE(bench.held(12));
}

TEST(driver_core, a_bus_line_the_port_refuses_gives_its_code_and_the_bus_keeps_its_defaults)
{
    PinBench bench;
    bench.m_i2c.m_configure_result = PDI_ERR_NOT_SUPPORTED;
    bench.declare("i2c0 bus i2c sda=12 scl=13\ni2c0 0x50 testchip\n");

    ASSERT_EQ(bench.busResult(SERIAL_IFACE_I2C), (int)PDI_ERR_NOT_SUPPORTED);
    ASSERT_TRUE(bench.held(4));
    ASSERT_FALSE(bench.held(12));
}

TEST(driver_core, a_bus_line_on_a_bus_that_takes_none_is_refused_as_not_supported)
{
    PinBench bench;
    bench.declare("i2s0 bus i2s sck=9\n");

    ASSERT_EQ(bench.busResult(SERIAL_IFACE_I2S), (int)PDI_ERR_NOT_SUPPORTED);
    ASSERT_FALSE(bench.held(9));
}

TEST(driver_core, a_line_whose_pins_an_earlier_line_holds_is_refused_as_busy)
{
    PinBench bench;
    bench.declare("i2c0 0x50 testchip\ni2s0 rx inmp441 sck=4 ws=10 sd=11\n");

    const driver_device_t *mic = bench.find(SERIAL_IFACE_I2S, 0);
    ASSERT_NOT_NULL(mic);
    ASSERT_EQ((int)mic->m_state, (int)DEVICE_STATE_FAILED);
    ASSERT_EQ((int)mic->m_result, (int)PDI_ERR_BUSY);
    ASSERT_EQ(bench.m_i2s.m_configures, 0);
    ASSERT_TRUE(bench.held(4));
    ASSERT_FALSE(bench.held(10));
}

TEST(driver_core, a_bus_line_is_taken_before_any_device_line_that_wants_its_pins)
{
    PinBench bench;
    bench.declare("i2s0 rx inmp441 sck=12 ws=10 sd=11\ni2c0 bus i2c sda=12 scl=13\n");

    ASSERT_EQ(bench.busResult(SERIAL_IFACE_I2C), (int)PDI_OK);
    const driver_device_t *mic = bench.find(SERIAL_IFACE_I2S, 0);
    ASSERT_NOT_NULL(mic);
    ASSERT_EQ((int)mic->m_state, (int)DEVICE_STATE_FAILED);
    ASSERT_EQ((int)mic->m_result, (int)PDI_ERR_BUSY);
    ASSERT_TRUE(bench.held(12));
    ASSERT_TRUE(bench.held(13));
    ASSERT_FALSE(bench.held(10));
}

TEST(driver_core, a_line_refused_for_its_pins_is_bound_once_they_are_free)
{
    PinBench bench;
    bench.declare("i2c0 0x50 testchip\ni2s0 rx inmp441 sck=4 ws=10 sd=11\n");
    ASSERT_EQ((int)bench.find(SERIAL_IFACE_I2S, 0)->m_state, (int)DEVICE_STATE_FAILED);

    bench.declare("i2s0 rx inmp441 sck=4 ws=10 sd=11\n");

    ASSERT_EQ((int)bench.find(SERIAL_IFACE_I2S, 0)->m_state, (int)DEVICE_STATE_BOUND);
    ASSERT_TRUE(bench.held(4));
}

TEST(driver_core, a_second_bus_line_for_one_bus_is_refused_as_already_there)
{
    PinBench bench;
    bench.declare("i2c0 bus i2c sda=12 scl=13\ni2c0 bus i2c sda=14 scl=15\n");

    ASSERT_EQ(bench.busResult(SERIAL_IFACE_I2C), (int)PDI_OK);
    ASSERT_TRUE(bench.held(12));
    ASSERT_FALSE(bench.held(14));
}

TEST(driver_core, a_bus_line_naming_one_pin_twice_or_one_past_the_held_range_is_refused_as_invalid)
{
    PinBench bench;
    bench.declare("i2c0 bus i2c sda=12 scl=12\ni2c0 0x50 testchip\n");
    ASSERT_EQ(bench.busResult(SERIAL_IFACE_I2C), (int)PDI_ERR_INVALID_ARG);
    ASSERT_FALSE(bench.held(12));
    ASSERT_TRUE(bench.held(4));

    bench.declare("i2c0 bus i2c sda=12 scl=64\n");
    ASSERT_EQ(bench.busResult(SERIAL_IFACE_I2C), (int)PDI_ERR_INVALID_ARG);
    ASSERT_FALSE(bench.held(12));
}

#ifdef ENABLE_SPI_BUS
TEST(driver_core, a_bus_line_naming_a_pin_another_bus_line_holds_is_refused_as_busy)
{
    PinBench bench;
    bench.declare("spi0 bus spi sck=12 miso=13 mosi=14\ni2c0 bus i2c sda=12 scl=15\n");

    ASSERT_EQ(bench.busResult(SERIAL_IFACE_SPI), (int)PDI_OK);
    ASSERT_EQ(bench.busResult(SERIAL_IFACE_I2C), (int)PDI_ERR_BUSY);
    ASSERT_FALSE(bench.held(15));
}

TEST(driver_core, a_bus_line_whose_default_pins_another_bus_line_holds_is_refused_and_put_back)
{
    PinBench bench;
    bench.declare("spi0 bus spi sck=4 miso=13 mosi=14\ni2c0 bus i2c\n");

    ASSERT_EQ(bench.busResult(SERIAL_IFACE_SPI), (int)PDI_OK);
    ASSERT_EQ(bench.busResult(SERIAL_IFACE_I2C), (int)PDI_ERR_BUSY);
    ASSERT_TRUE(bench.held(4));
    ASSERT_FALSE(bench.held(5));
    bench.m_i2c.m_probed.clear();
    bench.scan();
    ASSERT_TRUE(bench.m_i2c.m_probed.empty());
}
#endif

TEST(driver_core, a_bound_device_whose_pins_a_new_earlier_line_takes_is_let_go_as_busy)
{
    PinBench bench;
    bench.declare("i2s0 rx inmp441 sck=4 ws=10 sd=11\n");
    ASSERT_EQ((int)bench.find(SERIAL_IFACE_I2S, 0)->m_state, (int)DEVICE_STATE_BOUND);

    bench.declare("i2c0 0x50 testchip\ni2s0 rx inmp441 sck=4 ws=10 sd=11\n");

    const driver_device_t *mic = bench.find(SERIAL_IFACE_I2S, 0);
    ASSERT_EQ((int)mic->m_state, (int)DEVICE_STATE_FAILED);
    ASSERT_EQ((int)mic->m_result, (int)PDI_ERR_BUSY);
    ASSERT_NULL(mic->m_instance);
}

TEST(driver_core, a_driver_refusal_is_kept_on_the_line_with_its_code)
{
    PinBench bench;
    bench.m_i2s.m_noise = false;
    bench.declare("i2s0 rx inmp441\n");

    const driver_device_t *mic = bench.find(SERIAL_IFACE_I2S, 0);
    ASSERT_NOT_NULL(mic);
    ASSERT_EQ((int)mic->m_result, (int)PDI_ERR_NOT_FOUND);
    ASSERT_TRUE(bench.held(6));
}
#endif

TEST(driver_core, the_serial_service_runs_a_device_manager)
{
    ASSERT_NOT_NULL(serviceManager());
}

#ifdef ENABLE_I2S_BUS
TEST(driver_core, a_bound_mic_appears_under_dev_snd)
{
    ServiceBench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5\n");
    bench.m_mgr->trigger();
    pditest::Shell sh;

    ASSERT_TRUE(pditest::saw(sh.run("ls /dev"), "snd"));
    ASSERT_TRUE(pditest::saw(sh.run("ls /dev/snd"), "pcmC0D0c"));
}

TEST(driver_core, reading_the_capture_node_gives_samples)
{
    ServiceBench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5\n");
    bench.m_mgr->trigger();

    std::string got;
    int total = __i_fs.readFile("/dev/snd/pcmC0D0c", 16, [&](char *data, uint32_t size) -> bool {
        got.append(data, size);
        return true;
    });

    ASSERT_EQ(total, DEVFS_STREAM_READ_MAX);
    ASSERT_EQ((int)got.size(), DEVFS_STREAM_READ_MAX);
    ASSERT_NE(got, std::string(DEVFS_STREAM_READ_MAX, '\0'));
}

TEST(driver_core, the_capture_node_cannot_be_written)
{
    ServiceBench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5\n");
    bench.m_mgr->trigger();

    ASSERT_LT(__i_fs.writeFile("/dev/snd/pcmC0D0c", "x", 1), 0);
}

TEST(driver_core, a_mic_that_goes_takes_its_dev_node_with_it)
{
    ServiceBench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5\n");
    bench.m_mgr->trigger();
    writeDevicesConf("");
    bench.m_mgr->trigger();
    pditest::Shell sh;

    ASSERT_FALSE(pditest::saw(sh.run("ls /dev"), "snd"));
    ASSERT_FALSE(__i_fs.isFileExist("/dev/snd/pcmC0D0c"));
}
#endif

#if defined(ENABLE_I2S_BUS) && defined(ENABLE_STORAGE_SERVICE)
namespace
{

/**
 * The whole of a file, read back.
 */
std::string fileBytes(const char *path)
{
    std::string out;
    __i_fs.readFile(path, 256, [&](char *data, uint32_t size) -> bool {
        out.append(data, size);
        return true;
    });
    return out;
}

uint32_t le32(const std::string &b, size_t at)
{
    return (uint8_t)b[at] | ((uint8_t)b[at + 1] << 8) | ((uint8_t)b[at + 2] << 16) | ((uint32_t)(uint8_t)b[at + 3] << 24);
}

uint16_t le16(const std::string &b, size_t at)
{
    return (uint16_t)((uint8_t)b[at] | ((uint8_t)b[at + 1] << 8));
}

/**
 * Runs scheduled tasks until the file reaches the size, or a second passes.
 */
void pumpUntilSize(const char *path, int64_t size)
{
    uint32_t began = __i_dvc_ctrl.millis_now();
    while (__i_fs.getFileSize(path) < size && (__i_dvc_ctrl.millis_now() - began) < 1000)
    {
        __task_scheduler.handle_tasks();
    }
    for (int i = 0; i < 5; i++)
    {
        __task_scheduler.handle_tasks();
    }
}

void removeFile(const char *path)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    fs->beginPrivileged();
    if (fs->isFileExist(path))
    {
        fs->deleteFile(path);
    }
    fs->endPrivileged();
}

} // namespace

TEST(driver_core, arecord_writes_a_wav_file_of_the_length_asked_for)
{
    ServiceBench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5 rate=8000\n");
    bench.m_mgr->trigger();
    removeFile("/rec.wav");
    pditest::Shell sh;

    sh.run("arecord -d 1 /rec.wav");
    pumpUntilSize("/rec.wav", 44 + 16000);
    std::string out = sh.terminal().captured();
    std::string wav = fileBytes("/rec.wav");
    removeFile("/rec.wav");

    ASSERT_EQ((int)wav.size(), 44 + 16000);
    ASSERT_EQ(wav.substr(0, 4), std::string("RIFF"));
    ASSERT_EQ((int)le32(wav, 4), 36 + 16000);
    ASSERT_EQ(wav.substr(8, 8), std::string("WAVEfmt "));
    ASSERT_EQ((int)le32(wav, 16), 16);
    ASSERT_EQ((int)le16(wav, 20), 1);
    ASSERT_EQ((int)le16(wav, 22), 1);
    ASSERT_EQ((int)le32(wav, 24), 8000);
    ASSERT_EQ((int)le32(wav, 28), 16000);
    ASSERT_EQ((int)le16(wav, 32), 2);
    ASSERT_EQ((int)le16(wav, 34), 16);
    ASSERT_EQ(wav.substr(36, 4), std::string("data"));
    ASSERT_EQ((int)le32(wav, 40), 16000);
    ASSERT_TRUE(pditest::saw(out, "recorded 16000 bytes"));
}

TEST(driver_core, arecord_writes_its_file_as_the_user_who_started_it_whoever_typed_last)
{
    ServiceBench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5 rate=8000\n");
    bench.m_mgr->trigger();
    __i_fs.beginPrivileged();
    if (__i_fs.isDirExist("/rec_user")) __i_fs.deleteDirectory("/rec_user");
    __i_fs.createDirectory("/rec_user");
    __i_fs.setFileOwner("/rec_user", 1000, 1000);
    __i_fs.endPrivileged();
    pditest::Shell root;
    pditest::Shell user(1000, 1000);

    user.run("arecord -d 1 /rec_user/rec.wav");
    root.run("pwd");
    pumpUntilSize("/rec_user/rec.wav", 44 + 16000);

    file_info_t meta;
    ASSERT_EQ((int)__i_fs.getFileMeta("/rec_user/rec.wav", meta), (int)PDI_OK);
    __i_fs.beginPrivileged();
    __i_fs.deleteDirectory("/rec_user");
    __i_fs.endPrivileged();
    ASSERT_EQ((int)meta.m_uid, 1000);
    ASSERT_EQ((int)meta.m_size, 44 + 16000);
}

TEST(driver_core, arecord_stopped_early_keeps_a_header_that_matches_what_was_recorded)
{
    ServiceBench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5 rate=8000\n");
    bench.m_mgr->trigger();
    removeFile("/rec.wav");
    pditest::Shell sh;

    sh.run("arecord -d 30 /rec.wav");
    uint32_t began = __i_dvc_ctrl.millis_now();
    while ((__i_dvc_ctrl.millis_now() - began) < 50)
    {
        __task_scheduler.handle_tasks();
    }
    const uint8_t ctrl_c = 0x03;
    sh.type(&ctrl_c, 1);
    std::string wav = fileBytes("/rec.wav");
    removeFile("/rec.wav");

    ASSERT_GT((int)wav.size(), 44);
    ASSERT_LT((int)wav.size(), 44 + 480000);
    ASSERT_EQ((int)le32(wav, 40), (int)wav.size() - 44);
    ASSERT_EQ((int)le32(wav, 4), (int)wav.size() - 8);
}

TEST(driver_core, arecord_ends_cleanly_when_the_mic_goes_mid_recording)
{
    ServiceBench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5 rate=8000\n");
    bench.m_mgr->trigger();
    removeFile("/rec.wav");
    pditest::Shell sh;

    sh.run("arecord -d 30 /rec.wav");
    uint32_t began = __i_dvc_ctrl.millis_now();
    while ((__i_dvc_ctrl.millis_now() - began) < 50)
    {
        __task_scheduler.handle_tasks();
    }
    writeDevicesConf("");
    bench.m_mgr->trigger();
    began = __i_dvc_ctrl.millis_now();
    while ((__i_dvc_ctrl.millis_now() - began) < 50)
    {
        __task_scheduler.handle_tasks();
    }
    std::string out = sh.terminal().captured();
    std::string wav = fileBytes("/rec.wav");
    removeFile("/rec.wav");

    ASSERT_TRUE(pditest::saw(out, "capture device gone"));
    ASSERT_GT((int)wav.size(), 44);
    ASSERT_EQ((int)le32(wav, 40), (int)wav.size() - 44);
}

TEST(driver_core, arecord_names_the_code_when_the_bus_fails_mid_recording)
{
    ServiceBench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5 rate=8000\n");
    bench.m_mgr->trigger();
    removeFile("/rec.wav");
    pditest::Shell sh;

    sh.run("arecord -d 30 /rec.wav");
    uint32_t began = __i_dvc_ctrl.millis_now();
    while ((__i_dvc_ctrl.millis_now() - began) < 50)
    {
        __task_scheduler.handle_tasks();
    }
    bench.m_i2s.m_read_result = PDI_ERR_BUSY;
    began = __i_dvc_ctrl.millis_now();
    while ((__i_dvc_ctrl.millis_now() - began) < 50)
    {
        __task_scheduler.handle_tasks();
    }
    std::string out = sh.terminal().captured();
    std::string wav = fileBytes("/rec.wav");
    removeFile("/rec.wav");

    ASSERT_TRUE(pditest::saw(out, "capture device failed : -10"));
    ASSERT_GT((int)wav.size(), 44);
    ASSERT_EQ((int)le32(wav, 40), (int)wav.size() - 44);
}

TEST(driver_core, arecord_refuses_a_recording_the_filesystem_has_no_room_for)
{
    ServiceBench bench;
    writeDevicesConf("i2s0 rx inmp441 sck=3 ws=4 sd=5 rate=8000\n");
    bench.m_mgr->trigger();
    removeFile("/rec.wav");
    pditest::Shell sh;

    std::string out = sh.run("arecord -d 3600 /rec.wav");

    ASSERT_TRUE(pditest::saw(out, "not enough space"));
    ASSERT_FALSE(__i_fs.isFileExist("/rec.wav"));
}

TEST(driver_core, arecord_without_a_capture_device_says_so)
{
    ServiceBench bench;
    removeFile("/rec.wav");
    pditest::Shell sh;

    std::string out = sh.run("arecord -d 1 /rec.wav");

    ASSERT_TRUE(pditest::saw(out, "no capture device"));
    ASSERT_FALSE(__i_fs.isFileExist("/rec.wav"));
}

TEST(driver_core, arecord_needs_a_duration_and_a_file)
{
    ServiceBench bench;
    pditest::Shell sh;

    sh.run("arecord /rec.wav");
    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_ARGS_MISSING);

    sh.run("arecord -d 0 /rec.wav");
    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_INVAL);
}
#endif

TEST(driver_core, a_stopped_serial_service_lets_go_of_every_device_and_scans_again_once_started)
{
    ServiceBench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    bench.scan();
    ASSERT_NOT_NULL(bench.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS));
    uint8_t tasks = __serial_service.getServiceTaskCount();

    __serial_service.stopService();
    int removes = g_removes;
    ASSERT_NULL(__serial_service.devices());

    __serial_service.initService();
    bench.m_mgr = __serial_service.devices();

    ASSERT_EQ(removes, 1);
    ASSERT_NOT_NULL(bench.m_mgr);
    ASSERT_EQ((int)__serial_service.getServiceTaskCount(), (int)tasks);
}

TEST(driver_core, sysfs_lists_the_bus_of_each_interface_the_port_has)
{
    ServiceBench bench;
    pditest::Shell sh;

    std::string out = sh.run("ls /sys/bus");

    ASSERT_TRUE(pditest::saw(out, "i2c"));
    ASSERT_TRUE(pditest::saw(out, "usb"));
}

TEST(driver_core, a_found_device_has_a_directory_under_its_bus)
{
    ServiceBench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    bench.scan();
    pditest::Shell sh;

    ASSERT_TRUE(pditest::saw(sh.run("ls /sys/bus/i2c/devices"), "0-0050"));
    ASSERT_TRUE(pditest::saw(sh.run("cat /sys/bus/i2c/devices/0-0050/driver"), "testchip"));
    ASSERT_TRUE(pditest::saw(sh.run("cat /sys/bus/i2c/devices/0-0050/state"), "bound"));
}

TEST(driver_core, a_usb_device_reports_its_ids_under_sysfs)
{
    ServiceBench bench;
    bench.m_usb.plug(2, 0x046d, 0xc31c, USB_CLASS_HID);
    bench.scan();
    pditest::Shell sh;

    ASSERT_TRUE(pditest::saw(sh.run("cat /sys/bus/usb/devices/1-2/idVendor"), "046d"));
    ASSERT_TRUE(pditest::saw(sh.run("cat /sys/bus/usb/devices/1-2/idProduct"), "c31c"));
    ASSERT_TRUE(pditest::saw(sh.run("cat /sys/bus/usb/devices/1-2/bDeviceClass"), "03"));
}

TEST(driver_core, a_device_that_leaves_loses_its_sysfs_directory)
{
    ServiceBench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    bench.scan();
    bench.m_i2c.m_chips.clear();
    bench.scan();
    pditest::Shell sh;

    ASSERT_FALSE(pditest::saw(sh.run("ls /sys/bus/i2c/devices"), "0-0050"));
}

TEST(driver_core, sysfs_lists_the_drivers_of_each_bus)
{
    ServiceBench bench;
    pditest::Shell sh;

    ASSERT_TRUE(pditest::saw(sh.run("ls /sys/bus/i2c/drivers"), "testchip"));
    ASSERT_TRUE(pditest::saw(sh.run("ls /sys/bus/usb/drivers"), "testhid"));
}

TEST(driver_core, writing_rescan_looks_for_devices_at_once)
{
    ServiceBench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    pditest::Shell sh;

    sh.run("echo 1 > /sys/bus/i2c/rescan");

    ASSERT_NOT_NULL(bench.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS));
}

TEST(driver_core, udevadm_info_lists_each_device_with_its_driver)
{
    ServiceBench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    bench.scan();
    pditest::Shell sh;

    std::string out = sh.run("udevadm info");

    ASSERT_TRUE(pditest::saw(out, "i2c0"));
    ASSERT_TRUE(pditest::saw(out, "0-0050"));
    ASSERT_TRUE(pditest::saw(out, "testchip"));
    ASSERT_TRUE(pditest::saw(out, "bound"));
}

TEST(driver_core, udevadm_trigger_binds_what_was_plugged_since_the_last_scan)
{
    ServiceBench bench;
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    pditest::Shell sh;

    sh.run("udevadm trigger");

    const driver_device_t *dev = bench.find(SERIAL_IFACE_I2C, TESTCHIP_ADDRESS);
    ASSERT_NOT_NULL(dev);
    ASSERT_EQ((int)dev->m_state, (int)DEVICE_STATE_BOUND);
}

TEST(driver_core, udevadm_refuses_a_verb_it_does_not_know)
{
    ServiceBench bench;
    pditest::Shell sh;

    sh.run("udevadm sideways");

    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_OPT);
}

TEST(driver_core, udevadm_monitor_prints_each_device_that_comes_and_goes)
{
    ServiceBench bench;
    pditest::Shell sh;
    sh.run("udevadm monitor");

    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    bench.scan();
    bench.m_i2c.m_chips.clear();
    bench.scan();
    std::string out = sh.terminal().captured();

    ASSERT_TRUE(pditest::saw(out, "add i2c0 0-0050 testchip"));
    ASSERT_TRUE(pditest::saw(out, "remove i2c0 0-0050"));
}

TEST(driver_core, udevadm_monitor_stops_on_ctrl_c)
{
    ServiceBench bench;
    pditest::Shell sh;
    sh.run("udevadm monitor");

    const uint8_t ctrl_c = 0x03;
    sh.type(&ctrl_c, 1);
    sh.terminal().forget();
    bench.plugChip(TESTCHIP_ADDRESS, TESTCHIP_ID);
    bench.scan();

    ASSERT_FALSE(pditest::saw(sh.terminal().captured(), "add i2c0"));
}

#endif
