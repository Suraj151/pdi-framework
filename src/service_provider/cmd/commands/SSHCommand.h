/*********************************** SSH Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SSH_COMMAND_H_
#define _SSH_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_SSH_SERVICE

#include <utility/crypto/asymmetric/ed25519/ed25519.h>

/**
 * SSH command
 *
 * e.g. if we want to generate a key pair using SSH command, we can use the following command:
 * ssh q={SSHCommandQuery enum number}, t={SSHKeyType enum number}
 */
struct SSHCommand : public CommandBase {

	/* SSH Query options */
	enum SSHCommandQuery{
		SSH_COMMAND_QUERY_MIN = 0,
		SSH_COMMAND_QUERY_KEYGEN,
		SSH_COMMAND_QUERY_MAX
	};

	/* Constructor */
	SSHCommand(){
		Clear();
		SetCommand(CMD_NAME_SSH);
		AddOption(CMD_OPTION_NAME_Q);
		AddOption(CMD_OPTION_NAME_T);
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

		cmd_result_t result = CMD_RESULT_OK;
		SSHCommandQuery sshq = SSH_COMMAND_QUERY_MAX;
		SSHKeyAlgorithm sshk = SSH_KEY_ALGO_MAX;
		CommandOption *cmdoptn = nullptr;
		
		cmdoptn = RetrieveOption(CMD_OPTION_NAME_Q);
		if( nullptr != cmdoptn ){
			sshq = (SSHCommandQuery)StringToUint16(cmdoptn->optionval, cmdoptn->optionvalsize);
		}

		cmdoptn = RetrieveOption(CMD_OPTION_NAME_T);
		if( nullptr != cmdoptn ){
			sshk = (SSHKeyAlgorithm)StringToUint16(cmdoptn->optionval, cmdoptn->optionvalsize);
		}

		if( nullptr != m_terminal && 
			sshq > SSH_COMMAND_QUERY_MIN && sshq < SSH_COMMAND_QUERY_MAX 
			&& sshk > SSH_KEY_ALGO_MIN && sshk < SSH_KEY_ALGO_MAX ){

			if( sshq == SSH_COMMAND_QUERY_KEYGEN ){

				// Call the key generation function
				uint8_t pubkey[ED25519_PUBKEY_SIZE]; // ED25519 public key size
				uint8_t privkey[ED25519_PRIVKEY_SIZE]; // ED25519 private key size
				uint8_t seed[ED25519_SEED_SIZE]; // Optional seed for key generation.
				memset(pubkey, 0, sizeof(pubkey));
				memset(privkey, 0, sizeof(privkey));
				memset(seed, 0, sizeof(seed));

				if(sshk == SSH_KEY_ALGO_ED25519){

					// Generate a random seed for ED25519 key generation
					genUniqueKey((char*)seed, sizeof(seed));
					
					// Create ED25519 key pair
					ed25519_create_keypair(pubkey, privkey, seed); // Generate key pair

					// Save the generated keys to the file system
					const char* homedir = __i_fs.getHomeDirectory();

					if (homedir) {
						// Create the directory if it doesn't exist
						char sshdir[50]; memset(sshdir, 0, sizeof(sshdir));
						char privkey_path[75]; memset(privkey_path, 0, sizeof(privkey_path));
						char seed_path[75]; memset(seed_path, 0, sizeof(seed_path));
						char pubkey_path[75]; memset(pubkey_path, 0, sizeof(pubkey_path));
						int iStatus = 0;

						snprintf(sshdir, sizeof(sshdir), "%s/%s", (strlen(homedir) > 1 ? homedir : ""), SSH_DEFAULT_DIR);
						if (!__i_fs.isDirectory(sshdir)) {
							iStatus = __i_fs.createDirectory(sshdir);
						}

						if( iStatus >=0 ){
							// Save private key
							snprintf(privkey_path, sizeof(privkey_path), "%s/%s", sshdir, SSH_KEY_ALGO_ED25519_STR);
							iStatus = __i_fs.writeFile(privkey_path, (const char*)privkey, sizeof(privkey));

							// Save public key
							snprintf(pubkey_path, sizeof(pubkey_path), "%s/%s.pub", sshdir, SSH_KEY_ALGO_ED25519_STR);
							iStatus = __i_fs.writeFile(pubkey_path, (const char*)pubkey, sizeof(pubkey));

							// Save seed
							snprintf(seed_path, sizeof(seed_path), "%s/%s.seed", sshdir, SSH_KEY_ALGO_ED25519_STR);
							iStatus = __i_fs.writeFile(seed_path, (const char*)seed, sizeof(seed));
						}

						if (iStatus >= 0) {
							m_terminal->putln();
							m_terminal->writeln_ro(RODT_ATTR("SSH keys generated successfully."));
						} else {
							result = CMD_RESULT_FAILED;
							m_terminal->putln();
							m_terminal->write_ro(RODT_ATTR("Failed to save SSH keys : "));
							m_terminal->writeln((int32_t)iStatus);
						}
					}
				}
			}
		}

		return result;
	}
};

#endif // ENABLE_SSH_SERVICE

#endif
