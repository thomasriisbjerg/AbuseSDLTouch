#ifndef __ONLINE_SERVICE_H
#define __ONLINE_SERVICE_H

#ifdef __QNXNTO__
#include <scoreloop/scoreloopcore.h>
#endif // __QNXNTO__

class OnlineService
{
public:
	OnlineService();
	~OnlineService();
	void init();
	void connect();
	void connected(SC_Error_t status);
	void update();

private:
#ifdef __QNXNTO__
	SC_InitData_t scoreloop_init_data;
    SC_Client_h scoreloop_client;
    SCUI_Client_h scoreloop_ui_client;
    SC_UserController_h scoreloop_user_controller;
    bool has_user_controller;
#endif // __QNXNTO__
};

extern OnlineService *onlineservice;

#endif // __ONLINE_SERVICE_H
