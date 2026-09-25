/* Adding a shell command
 *
 * This example illustrate adding your own command to the framework shell.
 * Once registered it works over serial, telnet and ssh just like the built in
 * ones, and tab completion and help pick it up on their own.
 */

#include <PdiStack.h>

#if defined(ENABLE_CMD_SERVICE)

#define CMD_NAME_HELLO  "hello"		/* shell name, keep it 7 characters or less */

/**
 * HelloCommand must be derived from CommandBase
 */
struct HelloCommand : public CommandBase {

	/**
	 * HelloCommand constructor.
	 * construct with the name the user will type.
	 * setAcceptArgsOptions tells the parser to hand us whatever was typed after it.
	 */
	HelloCommand(){
		Clear();
		SetCommand(CMD_NAME_HELLO);
		setAcceptArgsOptions(true);
	}

	/**
	 * register the command so the shell can find it by name.
	 * the shell asks this for a fresh instance every time the command runs.
	 */
	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_HELLO, [](void *arg)->void *{
			return pdiutil::safe_new<HelloCommand>();
		});
	}

	/**
	 * one line describing the command, this is what help prints
	 */
	const char* getUsage() const override {
		return RODT_ATTR("hello [name]  greet the given name");
	}

#ifdef ENABLE_AUTH_SERVICE
	/**
	 * ask for a logged in user before running.
	 * drop this override if the command should be open to everyone.
	 */
	bool needauth() override { return true; }
#endif

	/**
	 * this runs when the user types the command
	 */
	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_ERROR_NOTTY;
		}

		/**
		 * first positional argument.
		 * the parser leaves the size at -1 when nothing was typed, so check the
		 * size rather than the text being empty
		 */
		CommandOption *name = &m_options[0];

		m_terminal->putln();
		m_terminal->write_ro(RODT_ATTR("hello "));

		if( nullptr != name->optionval && 0 < name->optionvalsize ){
			m_terminal->write(name->optionval, name->optionvalsize);
		}else{
			m_terminal->write_ro(RODT_ATTR("world"));
		}

		m_terminal->writeln();
		return PDI_OK;
	}
};

#else
  #error "Command service is disabled ( in devices/DeviceConfig.h of framework library ). please enable(uncomment) it for this example"
#endif


void setup() {

	PdiStack.initialize();

	// register after initialization, the shell looks commands up by name when they run
	HelloCommand::RegisterCommand();
}

void loop() {
	PdiStack.serve();
}
