/****************************** Login Controller ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_LOGIN_CONTROLLER_
#define _WEB_SERVER_LOGIN_CONTROLLER_

#include "Controller.h"
#include <webserver/pages/LoginPage.h>
#include <webserver/pages/LogoutPage.h>
#include <webserver/pages/LoginConfigPage.h>
#include <service_provider/auth/AuthServiceProvider.h>
#ifdef ENABLE_STORAGE_SERVICE
#include <service_provider/user/UserStoreService.h>
#endif

/**
 * LoginController class
 */
class LoginController : public Controller {

	public:

		/**
		 * LoginController constructor
		 */
		LoginController():Controller("login"){
		}

		/**
		 * LoginController destructor
		 */
		~LoginController(){
		}

		/**
		 * register logins controller
		 *
		 */
		void boot( void ){

			if( nullptr != this->m_route_handler ){
				this->m_route_handler->register_route( WEB_SERVER_LOGIN_ROUTE, [&]() { this->handleLoginRoute(); } );
				this->m_route_handler->register_route( WEB_SERVER_LOGOUT_ROUTE, [&]() { this->handleLogoutRoute(); } );
	      this->m_route_handler->register_route( WEB_SERVER_LOGIN_CONFIG_ROUTE, [&]() { this->handleLoginConfigRoute(); }, AUTH_MIDDLEWARE );
			}
		}

		/**
		 * build html page with header, middle and footer part.
		 *
		 * @param	char*	_page
		 * @param	const char *	_pgm_page
		 * @param	bool|false	_enable_flash
		 * @param	char*|""	_message
		 * @param	FLASH_MSG_TYPE|ALERT_SUCCESS	_alert_type
		 * @param	bool|true	_enable_header_footer
		 * @param	int|PAGE_HTML_MAX_SIZE	_max_size
		 */
		void build_html(
      char* _page,
      const char * _pgm_page,
      bool _enable_flash=false,
      char* _message="",
      FLASH_MSG_TYPE _alert_type=ALERT_SUCCESS ,
      bool _enable_header_footer=true,
      int _max_size=PAGE_HTML_MAX_SIZE
    ){

      // memset( _page, 0, _max_size );

      if( _enable_header_footer ) concat_header_html( _page );
      CONTINUE_SEND_IN_CHUNK(_page);
      strcat_ro( _page, _pgm_page );
      if( _enable_flash )
      concat_flash_message_div( _page, _message, _alert_type );
      if( _enable_header_footer ) strcat_ro( _page, WEB_SERVER_FOOTER_HTML );
      CONTINUE_SEND_IN_CHUNK(_page);
    }

		/**
		 * build change password html. never renders an existing password.
		 *
		 * @param	char*	_page
		 * @param	bool|false	_is_error
		 * @param	bool|false	_enable_flash
		 * @param	const char*	_message
		 * @param	int|PAGE_HTML_MAX_SIZE	_max_size
		 */
		void build_login_config_html( char* _page, bool _is_error=false, bool _enable_flash=false, const char* _message=nullptr, const char* _username=nullptr, int _max_size=PAGE_HTML_MAX_SIZE ){

      char _empty[1] = {0};

      concat_header_html( _page );
      strcat_ro( _page, WEB_SERVER_LOGIN_CONFIG_PAGE_TOP );
      CONTINUE_SEND_IN_CHUNK(_page);

      concat_tr_input_html_tags( _page, RODT_ATTR("User:"), RODT_ATTR("usrnm"), (char*)( nullptr != _username ? _username : __auth_service.getUsername() ), LOGIN_CONFIGS_BUF_SIZE-1, (char*)"text", false, true );
      concat_tr_input_html_tags( _page, RODT_ATTR("Current Password:"), RODT_ATTR("cpswd"), _empty, LOGIN_CONFIGS_BUF_SIZE-1, (char*)"password" );
      concat_tr_input_html_tags( _page, RODT_ATTR("New Password:"), RODT_ATTR("npswd"), _empty, LOGIN_CONFIGS_BUF_SIZE-1, (char*)"password" );
      concat_tr_input_html_tags( _page, RODT_ATTR("Confirm Password:"), RODT_ATTR("rpswd"), _empty, LOGIN_CONFIGS_BUF_SIZE-1, (char*)"password" );
      concat_csrf_input_html_tag( _page );

      strcat_ro( _page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM );
      if( _enable_flash )
      concat_flash_message_div( _page, _is_error ? (char*)_message : HTML_SUCCESS_FLASH, _is_error ? ALERT_DANGER:ALERT_SUCCESS );
      strcat_ro( _page, WEB_SERVER_FOOTER_HTML );
      CONTINUE_SEND_IN_CHUNK(_page);
    }

		/**
		 * build and send change password page.
		 * when posted, verify current password and update the user store.
		 */
    void handleLoginConfigRoute( void ) {

      LogI("Handling Login Config route\n");

			if( nullptr == this->m_web_resource ||
					nullptr == this->m_web_resource->m_server ||
					nullptr == this->m_route_handler ){
				return;
			}

      bool _is_posted = false;
      bool _is_error = true;
      bool _is_changed = false;
      pdiutil::string _message = CHARPTR_WRAP("Invalid length error(4-24)");
      pdiutil::string _username = __auth_service.getUsername();

      if ( this->m_web_resource->m_server->hasArg("cpswd") &&
           this->m_web_resource->m_server->hasArg("npswd") &&
           this->m_web_resource->m_server->hasArg("rpswd") ) {

        _is_posted = true;

        pdiutil::string _current = this->m_web_resource->m_server->arg("cpswd");
        pdiutil::string _new = this->m_web_resource->m_server->arg("npswd");
        pdiutil::string _repeat = this->m_web_resource->m_server->arg("rpswd");

        if( _new.size() >= LOGIN_CONFIGS_BUF_SIZE || _new.size() <= MIN_ACCEPTED_ARG_SIZE ){

          _message = CHARPTR_WRAP("Invalid length error(4-24)");

        }else if( _new != _repeat ){

          _message = CHARPTR_WRAP("Passwords do not match.");

        }else if( !__auth_service.isAuthorized( _username.c_str(), _current.c_str() ) ){

          _message = CHARPTR_WRAP("Current password is wrong.");

        }else{

          _is_error = !this->updatePassword( _username.c_str(), _new.c_str() );
          _is_changed = !_is_error;

          if( _is_error ){
            _message = CHARPTR_WRAP("Could not update password.");
          }
        }
      }

      char* _page = pdiutil::safe_new_array<char>(PAGE_HTML_MAX_SIZE);
      if (nullptr == _page) return;

      if( _is_changed ){
        __web_session_manager.destroyByUsername( _username.c_str() );
        this->m_route_handler->send_inactive_session_headers();
      }

      BEGIN_SEND_IN_CHUNK(HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page);
      this->build_login_config_html( _page, _is_error, _is_posted, _message.c_str(), _username.c_str() );
      END_SENDING_CHUNK();

      pdiutil::safe_delete_array(_page);
    }

		/**
		 * build and send logout html page. also inactive session for client
		 */
    void handleLogoutRoute( void ) {

      LogI("Handling logout route\n");

			if( nullptr == this->m_web_resource ||
					nullptr == this->m_web_resource->m_server ||
					nullptr == this->m_route_handler ){
				return;
			}

      this->m_route_handler->has_active_session();
      this->m_route_handler->send_inactive_session_headers();

      char* _page = pdiutil::safe_new_array<char>(PAGE_HTML_MAX_SIZE);
      if (nullptr == _page) return;

      BEGIN_SEND_IN_CHUNK(HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page);
      this->build_html( _page, WEB_SERVER_LOGOUT_PAGE );
      END_SENDING_CHUNK();

      // this->m_web_resource->m_server->send( HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page );
      pdiutil::safe_delete_array(_page);
    }

		/**
		 * redirect the client to home, optionally issuing a session cookie.
		 *
		 * @param	const char*	_token
		 */
		void redirectToHome( const char* _token ){

      if( nullptr == this->m_web_resource || nullptr == this->m_web_resource->m_server ){
        return;
      }

      this->m_web_resource->m_server->addHeader(CHARPTR_WRAP(HTTP_HEADER_KEY_LOCATION), WEB_SERVER_HOME_ROUTE);
      this->m_web_resource->m_server->addHeader(CHARPTR_WRAP_RO(HTTP_HEADER_KEY_CACHE_CONTROL), CHARPTR_WRAP_RO(HTTP_HEADER_VALUE_NO_CACHE));

      if( nullptr != _token && nullptr != this->m_route_handler ){

        login_credential_table _creds;
        uint32_t _max_age = SERVER_COOKIE_MAX_AGE;

        if( nullptr != this->m_web_resource->m_db_conn &&
            this->m_web_resource->m_db_conn->get_login_credential_table( &_creds ) ){
          _max_age = _creds.cookie_max_age;
        }

        char _session_cookie[EW_COOKIE_BUFF_MAX_SIZE];
        this->m_route_handler->build_session_cookie( _session_cookie, _token, EW_COOKIE_BUFF_MAX_SIZE, true, _max_age );
        this->m_web_resource->m_server->addHeader(CHARPTR_WRAP_RO(HTTP_HEADER_KEY_SET_COOKIE), _session_cookie);
      }

      this->m_web_resource->m_server->send(HTTP_RESP_MOVED_PERMANENTLY);
    }

		/**
		 * update the password of a user in the user store, falling back to the
		 * database table when the store is not provisioned yet.
		 *
		 * @param	const char*	_username
		 * @param	const char*	_password
		 * @return	bool
		 */
		bool updatePassword( const char* _username, const char* _password ){

#ifdef ENABLE_STORAGE_SERVICE
      pdiutil::string _shadow = CHARPTR_WRAP(USER_STORE_SHADOW_PATH);
      if( __i_fs.isFileExist( _shadow.c_str() ) ){
        return __user_store_service.setPassword( _username, _password );
      }
#endif

      if( nullptr == this->m_web_resource || nullptr == this->m_web_resource->m_db_conn ){
        return false;
      }

      login_credential_table _creds;
      if( !this->m_web_resource->m_db_conn->get_login_credential_table( &_creds ) ){
        return false;
      }

      memset( _creds.password, 0, LOGIN_CONFIGS_BUF_SIZE );
      memcpy( _creds.password, _password, strlen(_password) );
      this->m_web_resource->m_db_conn->set_login_credential_table( &_creds );

      return true;
    }

		/**
		 * check login details and authenticate client to access all configs.
		 */
    void handleLoginRoute( void ) {

			if( nullptr == this->m_web_resource ||
					nullptr == this->m_web_resource->m_server ||
					nullptr == this->m_route_handler ){
				return;
			}

      if( this->m_route_handler->has_active_session() ){
        this->redirectToHome( nullptr );
        return;
      }

      bool _is_posted = ( this->m_web_resource->m_server->hasArg("username") &&
                          this->m_web_resource->m_server->hasArg("password") );
      pdiutil::string _message = CHARPTR_WRAP("Wrong Credentials.");

      if( _is_posted ){

        if( __web_session_manager.isLoginBlocked() ){

          _message = CHARPTR_WRAP("Too many attempts. Try later.");

        }else{

          pdiutil::string _username = this->m_web_resource->m_server->arg("username");
          pdiutil::string _password = this->m_web_resource->m_server->arg("password");

          if( __auth_service.isAuthorized( _username.c_str(), _password.c_str() ) ){

            uint16_t _uid = 0;
            uint16_t _gid = 0;
#ifdef ENABLE_STORAGE_SERVICE
            user_record_t _record;
            if( __user_store_service.findUserByName( _username.c_str(), _record ) ){
              _uid = _record.m_uid;
              _gid = _record.m_gid;
            }
#endif

            web_session_t *_session = __web_session_manager.create( _username.c_str(), _uid, _gid );

            if( nullptr != _session ){

              __web_session_manager.clearLoginFailures();
              this->m_route_handler->adopt_session( _session );
              this->redirectToHome( _session->m_token );

              LogS("Log in Successful\n");
              return;
            }

            _message = CHARPTR_WRAP("No session slot free.");

          }else{

            __web_session_manager.registerLoginFailure();
          }
        }
      }

      char* _page = pdiutil::safe_new_array<char>(PAGE_HTML_MAX_SIZE);
      if (nullptr == _page) return;

      BEGIN_SEND_IN_CHUNK(HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page);
      this->build_html( _page, WEB_SERVER_LOGIN_PAGE, _is_posted, (char*)_message.c_str(), ALERT_DANGER );
      END_SENDING_CHUNK();

      // this->m_web_resource->m_server->send( HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page );
      pdiutil::safe_delete_array(_page);
    }

};

#endif
