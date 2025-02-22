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
#include "DatabaseServiceProvider.h"

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

	bool initService() override;
	bool isAuthorized(const char *username, const char *password);

	void setAuthorized(bool auth);
	bool getAuthorized();

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
	 * @var	bool	m_isAuthorized
	 */
    bool m_isAuthorized;
};

extern AuthServiceProvider __auth_service;

#endif
