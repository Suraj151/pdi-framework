/******************************** Auth Services *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _AUTH_SERVICE_H_
#define _AUTH_SERVICE_H_

#include <service_provider/ServiceProvider.h>
#include <service_provider/database/DatabaseServiceProvider.h>

/**
 * AuthServiceProvider class
 */
class AuthServiceProvider : public ServiceProvider
{

public:
	/**
	 * AuthServiceProvider constructor
	 */
	AuthServiceProvider();

	/**
	 * AuthServiceProvider destructor
	 */
	~AuthServiceProvider();

	bool initService(void *arg = nullptr) override;

/**
 * Needs the stored credentials it authenticates against before it can
 * come up.
 */
uint8_t getServiceDependencies(service_t *_out, uint8_t _max) const override {
  uint8_t _n = 0;
  if( _max > _n ) _out[_n++] = SERVICE_DATABASE;
  return _n;
}

	bool isEssentialService() const override { return true; }
	bool isAuthorized(const char *username, const char *password);

	void setAuthorized(bool auth);

	/**
	 * Moves the current session to its user's home, as a login does, leaving it in
	 * place when the account names none.
	 */
	void enterHome();

	bool getAuthorized();
	const char* getUsername();
	void setVerifiedUsername(const char *username);

  	void printConfigToTerminal(iTerminalInterface *terminal) override;
private:

	/**
	 * @var	login_credential_table	login_credentials
	 */
    login_credential_table m_login_credentials;

	/**
	 * @var	bool	m_initStatus
	 */
    bool m_initStatus;

	/**
	 * @var	pdiutil::string	m_lastVerifiedUsername
	 */
    pdiutil::string m_lastVerifiedUsername;
};

extern AuthServiceProvider __auth_service;

#endif
