#include "stdio.h"
#include "string.h"
#include "onlineservice.h"
#include "setup.h"

#ifdef __QNXNTO__
#include "onlineservice_scoreloop.h"
#endif // __QNXNTO__

OnlineService::OnlineService() : has_user_controller(false)
{

}

OnlineService::~OnlineService()
{
	SC_Client_Release(scoreloop_client);
//	SCUI_Client_Release(scoreloop_ui_client);

	if (has_user_controller)
		SC_UserController_Release(scoreloop_user_controller);
}

void OnViewEvent(void *cookie, SC_Error_t status)
{
	OnlineService *onlineservice = (OnlineService *)cookie;

	//onlineservice->OnViewEvent();
}

void OnlineService::init()
{
    SC_InitData_Init(&scoreloop_init_data);
    scoreloop_init_data.runLoopType = SC_RUN_LOOP_TYPE_CUSTOM;

    const char *scoreloop_language = strcmp(flags.language, "french") == 0 ? "fr" : strcmp(flags.language, "german") == 0 ? "de" : "en";

    SC_Error_t newClientErrCode = SC_Client_New(&scoreloop_client, &scoreloop_init_data, scoreloop_gameid, scoreloop_game_secret, scoreloop_game_version, scoreloop_game_currency, scoreloop_language);

    if (!newClientErrCode == SC_OK)
    {
    	return; // handle error
    }

//    SC_Error_t newUIClientErrCode = SCUI_Client_New(&scoreloop_ui_client, scoreloop_client);

//    if (!newUIClientErrCode == SC_OK)
//    	return; // handle error

//    SCUI_Client_SetViewEventCallback(scoreloop_ui_client, OnViewEvent, this);
}

void RequestUserControllerCompleted(void *cookie, SC_Error_t status)
{
	OnlineService *onlineservice = (OnlineService *)cookie;

	onlineservice->connected(status);
}

void OnlineService::connect()
{
	SC_Error_t createUserResult = SC_Client_CreateUserController(scoreloop_client, &scoreloop_user_controller, RequestUserControllerCompleted, this);
	if (createUserResult != SC_OK)
	{
		return; // handle error
	}

	has_user_controller = true;

	SC_Error_t loadUserResult = SC_UserController_LoadUser(scoreloop_user_controller);
	if (loadUserResult != SC_OK)
	{
		SC_UserController_Release(scoreloop_user_controller);
		return; // handle error
	}
}

void OnlineService::connected(SC_Error_t status)
{
	if (status != SC_OK)
	{
		if (has_user_controller)
		{
			SC_UserController_Release(scoreloop_user_controller);
			has_user_controller = false;
		}
		return; // handle error
	}

	SC_Session_h session = SC_Client_GetSession(scoreloop_client);

	SC_User_h user = SC_Session_GetUser(session);

	SC_UserController_Release(scoreloop_user_controller);
	has_user_controller = false;
}

void OnlineService::update()
{
	SC_HandleCustomEvent(&scoreloop_init_data, SC_FALSE);
}

OnlineService *onlineservice = new OnlineService();
