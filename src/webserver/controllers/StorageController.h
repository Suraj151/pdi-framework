/*************************** Storage Controller ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_STORAGE_CONTROLLER_
#define _WEB_SERVER_STORAGE_CONTROLLER_

#include "Controller.h"
#include <webserver/pages/StorageListPage.h>

#define CURRENT_PATH_ATTRIBUTE	"?cp="

/**
 * StorageController class
 */
class StorageController : public Controller
{

public:
	/**
	 * StorageController constructor
	 */
	StorageController() : Controller("storage")
	{
	}

	/**
	 * StorageController destructor
	 */
	~StorageController()
	{
	}

	/**
	 * register storage controller
	 *
	 */
	void boot(void)
	{
		if (nullptr != this->m_route_handler)
		{
			this->m_route_handler->register_route(
				WEB_SERVER_STORAGE_LIST_ROUTE, [&]()
				{ this->handleStorageListRoute(); },
				AUTH_MIDDLEWARE);

			this->m_route_handler->register_route(
				WEB_SERVER_STORAGE_FILE_UPLOAD_ROUTE, [&]()
				{ this->handleStorageFileUploadRoute(); },
				AUTH_MIDDLEWARE);				

			this->m_route_handler->register_route(
				WEB_SERVER_STORAGE_FILE_LIST_ROUTE, [&]()
				{ this->handleStorageFileListRoute(); },
				AUTH_MIDDLEWARE);				

			this->m_route_handler->register_route(
				WEB_SERVER_STORAGE_FILE_DELETE_ROUTE, [&]()
				{ this->handleStorageFileDeleteRoute(); },
				AUTH_MIDDLEWARE);				
		}
		if (nullptr != this->m_web_resource->m_server)
		{
			this->m_web_resource->m_server->setStoragePath(__i_fs.getHomeDirectory());
		}
	}

	/**
	 * build storage html.
	 *
	 * @param	char*	_page
	 * @param	bool|false	_enable_flash
	 * @param	int|PAGE_HTML_MAX_SIZE	_max_size
	 */
	void build_storage_html(char *_page, bool _enable_flash = false, int _max_size = PAGE_HTML_MAX_SIZE)
	{
		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_server ||
			nullptr == this->m_web_resource->m_db_conn)
		{
			return;
		}

		memset(_page, 0, _max_size);
		strcat_ro(_page, WEB_SERVER_HEADER_HTML);
		strcat_ro(_page, WEB_SERVER_STORAGE_LIST_PAGE_TOP);

		// Prepare the path to fetch
		pdiutil::string currentpath = __i_fs.getHomeDirectory();
		__i_fs.appendFileSeparator(currentpath);
		if(this->m_web_resource->m_server->hasArg("cp")){
			pdiutil::string cp = this->m_web_resource->m_server->arg("cp");
			__i_fs.updatePathNotations(cp.c_str(), currentpath);
			currentpath.replace("//", "/"); // avoid double file seperator in path
			__i_fs.appendFileSeparator(currentpath);
		}

		// pdiutil::vector<file_info_t> itemlist;
		// int resultCode = __i_fs.getDirFileList(currentpath.c_str(), itemlist);
		
		// if(!(resultCode < 0)){
		if(true){

			strcat_ro(_page, HTML_TABLE_OPEN_TAG);
			concat_id_attribute(_page, RODT_ATTR("strg-tbl"));
			concat_style_attribute(_page, RODT_ATTR("width:92%;margin-bottom:10px;"));
			strcat_ro(_page, HTML_TAG_CLOSE_BRACKET);

			// char *_storage_table_heading[] = {"", "Name", "Size"};
			// concat_table_heading_row(_page, _storage_table_heading, 3, nullptr, nullptr, nullptr, RODT_ATTR("text-align:left"));

			// Add current pah and stat in row
			pdiutil::string stat;
			stat += pdiutil::to_string(__i_fs.getFreeSize());
			stat += (char*)CHARPTR_WRAP(" Bytes Free");
			char *_storage_path_row[] = {(char*)currentpath.c_str(), (char*)stat.c_str()};
			const char *_storage_path_row_colspan[] = {RODT_ATTR("3"), RODT_ATTR("1' style='width:80px;")};
			concat_table_data_row(_page, _storage_path_row, 2, nullptr, nullptr, nullptr, nullptr, _storage_path_row_colspan);

			// Add empty row
			char *_storage_empty_row[] = {"&nbsp;"};
			const char *_storage_empty_row_colspan[] = {RODT_ATTR("3")};
			concat_table_data_row(_page, _storage_empty_row, 1, nullptr, nullptr, nullptr, nullptr, _storage_empty_row_colspan);

			// Add loader empty row
			pdiutil::string loaderdiv = CHARPTR_WRAP("<div id='ldr' class='ldr'></div>");
			char *_storage_loader_row[] = {(char*)loaderdiv.c_str()};
			concat_table_data_row(_page, _storage_loader_row, 1, nullptr, nullptr, nullptr, nullptr, _storage_empty_row_colspan);

			// // Prepare temporary buffers
			// uint32_t filenamenavlen = strlen(WEB_SERVER_STORAGE_LIST_ROUTE) + strlen(CURRENT_PATH_ATTRIBUTE) + currentpath.length() + 2*FILE_NAME_MAX_SIZE + 100;
			// uint32_t tempbufferlen = max(filenamenavlen, (uint32_t)(max(strlen(SVG_ICON48_1416_PATH_FOLDER), strlen(SVG_ICON48_1216_PATH_FILE)) + 100));
			// char *tempbuffer = new char[tempbufferlen]; 
			// char *filenamenav = new char[filenamenavlen]; 

			// for (file_info_t item : itemlist) {

			// 	// avoid current directory
			// 	if( strcmp((const char*)item.name, ".") == 0 || strlen(_page) > (PAGE_HTML_MAX_SIZE - 500) ){
			// 		delete[] item.name;
			// 		continue;
			// 	}

			// 	// Build link element
			// 	memset(filenamenav, 0, filenamenavlen);
			// 	memset(tempbuffer, 0, tempbufferlen);
			// 	pdiutil::string _path = currentpath + item.name;
			// 	if( __i_fs.isDirectory(_path.c_str()) ){
			// 		strcat(tempbuffer, WEB_SERVER_STORAGE_LIST_ROUTE);
			// 		strcat(tempbuffer, CURRENT_PATH_ATTRIBUTE);
			// 	}
			// 	strncat(tempbuffer, _path.c_str(), _path.length());
			// 	concat_link_element(
			// 		filenamenav,
			// 		(const char*)tempbuffer,
			// 		(const char*)item.name
			// 	);

			// 	// Build svg element 
			// 	memset(tempbuffer, 0, tempbufferlen);
			// 	concat_svg_tag(
			// 		tempbuffer, 
			// 		item.type == FILE_TYPE_DIR ? SVG_ICON48_1416_PATH_FOLDER : SVG_ICON48_1216_PATH_FILE,
			// 		RODT_ATTR("margin-left:0;"),
			// 		item.type == FILE_TYPE_DIR ? RODT_ATTR("0 0 14 16") : RODT_ATTR("0 0 12 16"),
			// 		24,24
			// 	);

			// 	// Build table row and append
			// 	char filesize[16]; memset(filesize, 0, 16); Int64ToString(item.size, filesize, 15);
			// 	char *_storage_table_row_data[] = {
			// 		tempbuffer, 
			// 		filenamenav, 
			// 		filesize
			// 	};
			// 	concat_table_data_row(_page, _storage_table_row_data, 3, nullptr, nullptr, nullptr, nullptr);

			// 	// deallocates memory for items
			// 	delete[] item.name;
			// }

			// delete[] tempbuffer;
			// delete[] filenamenav;
			// itemlist.clear();
			
			strcat_ro(_page, HTML_TABLE_CLOSE_TAG);
		}

		strcat_ro(_page, WEB_SERVER_STORAGE_LIST_PAGE_BOTTOM);

		if (_enable_flash)
			concat_flash_message_div(_page, HTML_SUCCESS_FLASH, ALERT_SUCCESS);
		strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
	}

	/**
	 * build and send storage list page.
	 */
	void handleStorageListRoute(void)
	{
		LogI("Handling Storage list route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_db_conn ||
			nullptr == this->m_web_resource->m_server)
		{
			return;
		}

		char *_page = new char[PAGE_HTML_MAX_SIZE];
		this->build_storage_html(_page);

		this->m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page);
		delete[] _page;
	}

	/**
	 * handle upload, build and send storage list page.
	 */
	void handleStorageFileUploadRoute(void)
	{
		LogI("Handling Storage file upload route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_db_conn ||
			nullptr == this->m_web_resource->m_server)
		{
			return;
		}

		pdiutil::string loc = WEB_SERVER_HOME_ROUTE;
		pdiutil::string currentdir;
		if(this->m_web_resource->m_server->hasArg("loc")){
			loc = this->m_web_resource->m_server->arg("loc");
			currentdir = loc;

			// Remove path uri and atributes to get current path
			currentdir.replace(WEB_SERVER_STORAGE_LIST_ROUTE, "");
			currentdir.replace(CURRENT_PATH_ATTRIBUTE, "");
			__i_fs.appendFileSeparator(currentdir);
		}

		if(this->m_web_resource->m_server->hasArg("nf")){

			pdiutil::string nf = this->m_web_resource->m_server->arg("nf");

			// Move file to expected location
			if( currentdir.length() > 0 && __i_fs.isDirectory(currentdir.c_str()) && __i_fs.isFileExist(nf.c_str()) ){

				// prepare filepath in current directory
				pdiutil::string newfilepath = currentdir + __i_fs.basename(nf.c_str());

				// find new name if file already exist
				for (uint32_t i = 1; i < 1000; i++){

					if( !__i_fs.isFileExist(newfilepath.c_str()) ){
						break;
					}
					newfilepath = currentdir;
					newfilepath += '(' + pdiutil::to_string(i) + ')';
					newfilepath += __i_fs.basename(nf.c_str());
				}

				// Move file to expected path
				__i_fs.rename(nf.c_str(), newfilepath.c_str());
			}
		}

		if(this->m_web_resource->m_server->hasArg("nd")){

			pdiutil::string nd = this->m_web_resource->m_server->arg("nd");

			// Create folder to expected location
			if( currentdir.length() > 0 && __i_fs.isDirectory(currentdir.c_str()) && nd.length() > 0 ){

				// prepare folder path in current directory
				pdiutil::string newfolderpath = currentdir + nd.substr(0, FILE_NAME_MAX_SIZE);

				if ( !__i_fs.isDirExist(newfolderpath.c_str()) ){
					__i_fs.createDirectory(newfolderpath.c_str());
				}				
			}
		}

        this->m_web_resource->m_server->addHeader("Location", loc);
        this->m_web_resource->m_server->send(HTTP_RESP_MOVED_PERMANENTLY);
	}


	/**
	 * build and send storage file list page.
	 */
	void handleStorageFileListRoute(void)
	{
		LogI("Handling Storage file list route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_db_conn ||
			nullptr == this->m_web_resource->m_server)
		{
			return;
		}

		// Prepare the path to fetch
		pdiutil::string currentpath = __i_fs.getHomeDirectory();
		__i_fs.appendFileSeparator(currentpath);
		if(this->m_web_resource->m_server->hasArg("cp")){
			pdiutil::string cp = this->m_web_resource->m_server->arg("cp");
			__i_fs.updatePathNotations(cp.c_str(), currentpath);
			currentpath.replace("//", "/"); // avoid double file seperator in path
			__i_fs.appendFileSeparator(currentpath);
		}

		pdiutil::vector<file_info_t> itemlist;
		int resultCode = __i_fs.getDirFileList(currentpath.c_str(), itemlist);

		// Prepare temporary buffers
		uint32_t tempbufferlen = max(strlen(SVG_ICON48_1616_PATH_TRASH),max(strlen(SVG_ICON48_1416_PATH_FOLDER), strlen(SVG_ICON48_1216_PATH_FILE))) + 100;
		char *tempbuffer = new char[tempbufferlen]; 

		pdiutil::string jsonresp = "{";
		jsonresp += CHARPTR_WRAP("\"dsvg\":\"");

		// Build folder svg element 
		memset(tempbuffer, 0, tempbufferlen);
		concat_svg_tag(
			tempbuffer, 
			SVG_ICON48_1416_PATH_FOLDER,
			RODT_ATTR("margin-left:0;"),
			RODT_ATTR("0 0 14 16"),
			24,24
		);

		jsonresp += tempbuffer;
		jsonresp += CHARPTR_WRAP("\",\"fsvg\":\"");

		// Build file svg element 
		memset(tempbuffer, 0, tempbufferlen);
		concat_svg_tag(
			tempbuffer, 
			SVG_ICON48_1216_PATH_FILE,
			RODT_ATTR("margin-left:0;"),
			RODT_ATTR("0 0 12 16"),
			24,24
		);

		jsonresp += tempbuffer;
		jsonresp += CHARPTR_WRAP("\",\"tsvg\":\"");

		// Build trash svg element 
		memset(tempbuffer, 0, tempbufferlen);
		concat_svg_tag(
			tempbuffer, 
			SVG_ICON48_1616_PATH_TRASH,
			nullptr,
			nullptr,
			16,16, "#797979"
		);

		jsonresp += tempbuffer;
		jsonresp += CHARPTR_WRAP("\",\"lst\":[");

		if(!(resultCode < 0)){

			for (file_info_t item : itemlist) {

				// avoid current directory
				if( strcmp((const char*)item.name, ".") == 0 ){
					delete[] item.name;
					continue;
				}

				// Build link element
				memset(tempbuffer, 0, tempbufferlen);
				pdiutil::string _path = currentpath + item.name;
				if( __i_fs.isDirectory(_path.c_str()) ){
					strcat(tempbuffer, WEB_SERVER_STORAGE_LIST_ROUTE);
					strcat(tempbuffer, CURRENT_PATH_ATTRIBUTE);
				}
				strncat(tempbuffer, _path.c_str(), _path.length());

				jsonresp += "{\"n\":\"";
				jsonresp += item.name;
				jsonresp += "\",\"s\":\"";
				jsonresp += pdiutil::to_string(item.size);
				jsonresp += "\",\"t\":\"";
				jsonresp += item.type == FILE_TYPE_DIR ? "D":"F";
				jsonresp += "\",\"l\":\"";
				jsonresp += tempbuffer;
				jsonresp += "\"},";

				// deallocates memory for items
				delete[] item.name;
			}

			jsonresp.pop_back(); // remove last comma
			itemlist.clear();
		}

		delete[] tempbuffer;
		jsonresp += CHARPTR_WRAP("]}");

		this->m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_APPLICATION_JSON, jsonresp.c_str());
	}

	/**
	 * handle delete, build and send storage list page.
	 */
	void handleStorageFileDeleteRoute(void)
	{
		LogI("Handling Storage file delete route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_db_conn ||
			nullptr == this->m_web_resource->m_server)
		{
			return;
		}

		pdiutil::string loc = WEB_SERVER_HOME_ROUTE;
		if(this->m_web_resource->m_server->hasArg("df")){

			pdiutil::string df = this->m_web_resource->m_server->arg("df");
			pdiutil::string currentdir;

			if(this->m_web_resource->m_server->hasArg("loc")){
				loc = this->m_web_resource->m_server->arg("loc");
				currentdir = loc;

				// Remove path uri and atributes to get current path
				currentdir.replace(WEB_SERVER_STORAGE_LIST_ROUTE, "");
				currentdir.replace(CURRENT_PATH_ATTRIBUTE, "");
				__i_fs.appendFileSeparator(currentdir);
			}

			// Move file to expected location
			if( currentdir.length() > 0 && __i_fs.isDirectory(currentdir.c_str()) && 
				(__i_fs.isFileExist(df.c_str()) || __i_fs.isDirExist(df.c_str())) ){

				// delete file/dir
				if( __i_fs.isDirectory(df.c_str()) ){
					__i_fs.deleteDirectory(df.c_str());
				}else{
					__i_fs.deleteFile(df.c_str());
				}
			}
		}

        this->m_web_resource->m_server->addHeader("Location", loc);
        this->m_web_resource->m_server->send(HTTP_RESP_MOVED_PERMANENTLY);
	}

};

#endif
