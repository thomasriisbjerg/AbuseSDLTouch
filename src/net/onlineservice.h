#ifndef __ONLINE_SERVICE_H
#define __ONLINE_SERVICE_H

#ifdef __QNXNTO__
#include <scoreloop/scoreloopcore.h>
#endif // __QNXNTO__

class OnlineService;
class OnlineServiceTask;

typedef void (OnlineService::*OnlineServiceCallback)(SC_Error_t status);

enum OnlineServiceState
{
	DISCONNECTED,
	LOAD_USER,
	LOAD_USER_CONTEXT,
	CONNECTED,
	SUBMIT_USER_CONTEXT,
};

class OnlineService
{
public:
	OnlineService();
	~OnlineService();
	void init();
	void connect();
	void userLoaded(SC_Error_t status);
	void userContextLoaded(SC_Error_t status);
	void userContextSubmitted(SC_Error_t status);
	void onViewEvent(SC_Error_t status);
	void disconnect();
	void downloadSaveGame(const char *filename);
	void persistSaveGame(const char *filename);
	void update();
	void updateUI(bps_event_t *event);
	void showUI();
	void onViewClosed(void *cookie, SCUI_Result_t viewResult, const void *data);
	bool isConnected();
	OnlineServiceState getState();

private:
#ifdef __QNXNTO__
	SC_InitData_t scoreloop_init_data;
    SC_Client_h scoreloop_client;
    SCUI_Client_h scoreloop_ui_client;
    SC_UserController_h scoreloop_user_controller;
    SC_User_h scoreloop_user;
    bool has_user_controller;
    bool has_user;
    OnlineServiceState state;
#endif // __QNXNTO__
};

void OnlineServiceTaskCompleted(void *cookie, SC_Error_t status);

extern OnlineService *onlineservice;

#endif // __ONLINE_SERVICE_H
