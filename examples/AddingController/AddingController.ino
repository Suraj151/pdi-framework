/* Adding controller (available from third release)
 *
 * This example illustrate adding controller to framework to handle specific url path
 * framework recommends this design of request response model to implement for future use
 *
 */

#include <PdiStack.h>


#if defined(ENABLE_HTTP_SERVER)

/**
 * TestController class must be derived from Controller class
 */
class TestController : public Controller {

	public:

		/**
		 * TestController constructor.
     * construct with unique controller name to differentiate it from others
		 */
		TestController():Controller("test"){}

		/**
		 * register test controller
     *
     * argument includes url path, route handler method, middleware option.
     *
     * AUTH_MIDDLEWARE is middlware which enable auth check service to this route so that
     * handler method will only call if user is authenticated before with login credentials.
		 */
		void boot( void ){
			this->m_route_handler->register_route( "/test-route", [&]() { this->handleTestRoute(); }, AUTH_MIDDLEWARE );
		}

    /**
		 * build and send test page to client
		 */
    void handleTestRoute( void ) {

      LogI("Handling Test route\n");

      /**
       * take new dynamic array to build html response page
       * PAGE_HTML_MAX_SIZE defined in framework as 5000
       */
      char* _page = new char[PAGE_HTML_MAX_SIZE];
			memset( _page, 0, PAGE_HTML_MAX_SIZE );

      /**
       * first append header part of html to reponse
  		 */
			strcat_ro( _page, WEB_SERVER_HEADER_HTML );

      /**
       * then append body part of html to response
       * for demo purpose, dashboard card added with the help of html helpers available in framework
  		 */
			strcat_ro( _page, WEB_SERVER_MENU_CARD_PAGE_WRAP_TOP );
			concat_svg_menu_card( _page, WEB_SERVER_HOME_MENU_TITLE_DASHBOARD, SVG_ICON48_PATH_DASHBOARD, WEB_SERVER_DASHBOARD_ROUTE );
			strcat_ro( _page, WEB_SERVER_MENU_CARD_PAGE_WRAP_BOTTOM );

      /**
       * lastely append footer part of html to response
  		 */
			strcat_ro( _page, WEB_SERVER_FOOTER_HTML );

      /**
       * finally send response and deallocate page
  		 */
      this->m_web_resource->m_server->send( HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page );
      delete[] _page;
    }
};

/**
 * this should be defined before framework initialization
 */
TestController test_controller;
#else
  #error "HTTP server is disabled ( in config/Common.h of framework library ). please enable(uncomment) it for this example"
#endif


void setup() {
 PdiStack.initialize();
}

void loop() {
 PdiStack.serve();
}
