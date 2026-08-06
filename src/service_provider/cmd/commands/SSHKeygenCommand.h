/******************************* SSH Keygen Command **************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SSH_KEYGEN_COMMAND_H_
#define _SSH_KEYGEN_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_SSH_SERVICE

#include <utility/crypto/asymmetric/rsa/rsa.h>
#include <utility/SafeAlloc.h>

// SSH key storage helpers (defined in SSHServiceUtil.cpp)
namespace LWSSH {
    bool generate_ed25519_key(const char* dir);
    bool save_rsa_key(const rsa_key& key, const char* dir);
    void ssh_rng_fill(uint8_t* buf, size_t len);
}

/**
 * SSH keygen command
 *
 * Generates an ssh key pair of the requested algorithm in the requested
 * directory, e.g. sshkgen t={SSHKeyAlgorithm enum number},f={directory}
 */
struct SSHKeygenCommand : public CommandBase {

	/* Constructor */
	SSHKeygenCommand(){
		Clear();
		SetCommand(CMD_NAME_SSHKEYGEN);
		AddOption(CMD_OPTION_NAME_T);
		AddOption(CMD_OPTION_NAME_F);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_SSHKEYGEN, [](void *arg)->void *{
			return new SSHKeygenCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("sshkgen t=<algo>[,f=<dir>]  t=1 ed25519, t=2 rsa (slow). f prompted when absent");
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		if( terminputaction == CMD_TERM_INSEQ_CTRL_C ||
		    terminputaction == CMD_TERM_INSEQ_CTRL_Z ){
			return CMD_RESULT_ABORTED;
		}

		cmd_result_t result = CMD_RESULT_OK;
		SSHKeyAlgorithm sshk = SSH_KEY_ALGO_MAX;
		CommandOption *cmdoptn = nullptr;

		cmdoptn = RetrieveOption(CMD_OPTION_NAME_T);
		if( nullptr != cmdoptn ){
			sshk = (SSHKeyAlgorithm)StringToUint16(cmdoptn->optionval, cmdoptn->optionvalsize);
		}

		if( nullptr == m_terminal || sshk <= SSH_KEY_ALGO_MIN || sshk >= SSH_KEY_ALGO_MAX ){
			return CMD_RESULT_ARGS_ERROR;
		}

		CommandOption *diroptn = RetrieveOption(CMD_OPTION_NAME_F);
		bool hasdir = ( nullptr != diroptn && nullptr != diroptn->optionval && diroptn->optionvalsize );

		if( !hasdir && 0 == m_iterations ){
			holdOptionValue(CMD_OPTION_NAME_T);
			setWaitingForOption(CMD_OPTION_NAME_F);
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("key directory a] ~/.ssh  b] /etc/ssh (b is default) : "));
			return CMD_RESULT_INCOMPLETE;
		}

		pdiutil::string keydir = resolveKeyDir(diroptn);

		if( sshk == SSH_KEY_ALGO_ED25519 ){

			if( LWSSH::generate_ed25519_key(keydir.c_str()) ){
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("SSH keys generated in "));
				m_terminal->writeln(keydir.c_str());
			}else{
				result = CMD_RESULT_FAILED;
				m_terminal->putln();
				m_terminal->writeln_ro(RODT_ATTR("Failed to save SSH keys."));
			}
		}
		else if( sshk == SSH_KEY_ALGO_RSA_SHA256 || sshk == SSH_KEY_ALGO_RSA_SHA512 ){

			rsa_key *key = pdiutil::safe_new<rsa_key>();
			if( nullptr == key ){
				result = CMD_RESULT_FAILED;
				m_terminal->putln();
				m_terminal->writeln_ro(RODT_ATTR("Not enough memory for RSA key."));
			}else{

				m_terminal->putln();
				m_terminal->writeln_ro(RODT_ATTR("Generating RSA key, this may take a while..."));

				bn_set_yield_hook([](){ __i_dvc_ctrl.yield(); });
				bool gen = rsa_generate_keypair(key, SSH_RSA_KEY_BITS, LWSSH::ssh_rng_fill);
				bn_set_yield_hook(nullptr);

				if( gen && LWSSH::save_rsa_key(*key, keydir.c_str()) ){
					m_terminal->write_ro(RODT_ATTR("SSH keys generated in "));
					m_terminal->writeln(keydir.c_str());
				}else{
					result = CMD_RESULT_FAILED;
					m_terminal->writeln_ro(RODT_ATTR("Failed to generate/save RSA keys."));
				}

				pdiutil::safe_delete(key);
			}
		}

		return result;
	}

	/**
	 * @brief Resolve the key directory option into a usable path.
	 *
	 * Accepts the menu choices a (user ssh directory) and b (server ssh
	 * directory) or an explicit directory path. Defaults to the server one.
	 */
	pdiutil::string resolveKeyDir(CommandOption *diroptn){

		bool hasdir = ( nullptr != diroptn && nullptr != diroptn->optionval && diroptn->optionvalsize );
		bool ischoice = ( hasdir && 1 == diroptn->optionvalsize );

		if( ischoice && ( 'a' == diroptn->optionval[0] || 'A' == diroptn->optionval[0] ) ){

			const char* homedir = __i_fs.getHomeDirectory();
			pdiutil::string keydir = pdiutil::string(strlen(homedir) > 1 ? homedir : "");
			__i_fs.appendFileSeparator(keydir);
			keydir += CHARPTR_WRAP(SSH_DEFAULT_DIR);
			return keydir;
		}

		if( hasdir && !( ischoice && ( 'b' == diroptn->optionval[0] || 'B' == diroptn->optionval[0] ) ) ){

			return resolveArgPath(diroptn);
		}

		return pdiutil::string(CHARPTR_WRAP(SSH_HOST_KEY_DIR));
	}
};

#endif // ENABLE_SSH_SERVICE

#endif
