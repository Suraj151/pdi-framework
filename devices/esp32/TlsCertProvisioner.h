/********************** TLS Cert Provisioner ***********************************
This file is part of the pdi stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 7th June 2026
******************************************************************************/
#ifndef _TLS_CERT_PROVISIONER_H_
#define _TLS_CERT_PROVISIONER_H_

#include "esp32.h"

namespace TlsCertProvisioner {

enum KeyAlgo {
    KEY_ALGO_EC = 0,
    KEY_ALGO_RSA,
};

struct CertParams {
    KeyAlgo algo;
    int keySize;
    const char* subjectCn;
    const char* issuerCn;
    uint32_t ip_v4;
    const char* dns_name;
    uint32_t validityDays;
    bool isCa;

    CertParams()
        : algo(KEY_ALGO_EC),
          keySize(256),
          subjectCn(nullptr),
          issuerCn(nullptr),
          ip_v4(0),
          dns_name(nullptr),
          validityDays(0),
          isCa(false) {}
};

bool generateCert(const char* certPath,
                  const char* keyPath,
                  const CertParams& params);

bool ensureServerCert(const char* certPath,
                      const char* keyPath,
                      uint32_t ip_v4,
                      const char* dns_name);

}

#endif
