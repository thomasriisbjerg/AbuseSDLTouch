#include "onlineservice.h"
#include "stdio.h"
#include "string.h"
#include "specs.h"
#include "setup.h"
#include "ascii85.h"
#include <cassert>

#ifdef __QNXNTO__
#include "onlineservice_scoreloop.h"
#endif // __QNXNTO__

OnlineService::OnlineService() : has_user_controller(false), has_user(false), state(DISCONNECTED)
{
}

OnlineService::~OnlineService()
{
	SC_Client_Release(scoreloop_client);
	SCUI_Client_Release(scoreloop_ui_client);

	if (has_user_controller)
		SC_UserController_Release(scoreloop_user_controller);
	has_user_controller = false;

	if (has_user)
		SC_User_Release(scoreloop_user);
	has_user = false;
}

void OnViewEvent(void *cookie, SC_Error_t status)
{
	onlineservice->onViewEvent(status);
}

void OnlineService::onViewEvent(SC_Error_t status)
{
	//printf("onViewEvent: %i\n", status); fflush(stdout);
}

void OnlineService::init()
{
	//printf("online init\n"); fflush(stdout);
    SC_InitData_Init(&scoreloop_init_data);
    scoreloop_init_data.runLoopType = SC_RUN_LOOP_TYPE_CUSTOM;

    const char *scoreloop_language = strcmp(flags.language, "french") == 0 ? "fr" : strcmp(flags.language, "german") == 0 ? "de" : "en";

    SC_Error_t newClientErrCode = SC_Client_New(&scoreloop_client, &scoreloop_init_data, scoreloop_gameid, scoreloop_game_secret, scoreloop_game_version, scoreloop_game_currency, scoreloop_language);

    if (!newClientErrCode == SC_OK)
    {
    	//printf("online SC_Client_New failed with code %i\n", newClientErrCode); fflush(stdout);
    	return; // handle error
    }

    SC_Error_t newUIClientErrCode = SCUI_Client_New(&scoreloop_ui_client, scoreloop_client);

    if (!newUIClientErrCode == SC_OK)
    {
    	//printf("online SCUI_Client_New failed with code %i\n", newUIClientErrCode);
    	return; // handle error
    }

    SCUI_Client_SetViewEventCallback(scoreloop_ui_client, OnViewEvent, this);
}

void RequestUserControllerCompleted(void *cookie, SC_Error_t status)
{
	//printf("user controller completed state %i\n", onlineservice->getState()); fflush(stdout);
	switch (onlineservice->getState())
	{
	case LOAD_USER:
		onlineservice->userLoaded(status);
		break;
	case LOAD_USER_CONTEXT:
		onlineservice->userContextLoaded(status);
		break;
	case SUBMIT_USER_CONTEXT:
		onlineservice->userContextSubmitted(status);
		break;
	default:
		assert(false);
		break;
	}
}

void OnlineService::connect()
{
	//printf("onlineservice connect\n"); fflush(stdout);
	assert(has_user_controller == false);
	assert(state == DISCONNECTED);

	SC_Error_t createUserControllerResult = SC_Client_CreateUserController(scoreloop_client, &scoreloop_user_controller, RequestUserControllerCompleted, 0);
	if (createUserControllerResult != SC_OK)
	{
		//printf("SC_Client_CreateUserController failed with code %i\n", createUserControllerResult); fflush(stdout);
		return; // handle error
	}

	has_user_controller = true;

	SC_Error_t loadUserResult = SC_UserController_LoadUser(scoreloop_user_controller);
	if (loadUserResult == SC_OK)
	{
		state = LOAD_USER;
	}
	else
	{
		//printf("SC_UserController_LoadUser failed with code %i\n", loadUserResult); fflush(stdout);
		SC_UserController_Release(scoreloop_user_controller);
		state = DISCONNECTED;
		return; // handle error
	}
	printf("online connect done\n"); fflush(stdout);
}

void OnlineService::userLoaded(SC_Error_t status)
{
	//printf("online userLoaded\n"); fflush(stdout);
	assert(state == LOAD_USER);

	if (status != SC_OK)
	{
		if (has_user_controller)
		{
			SC_UserController_Release(scoreloop_user_controller);
			has_user_controller = false;
		}
		state = DISCONNECTED;
		//printf("SC_UserController_LoadUser failed with code %i\n", status); fflush(stdout);
		return; // handle error
	}

	assert(has_user_controller);

	SC_Error_t loadUserContextResult = SC_UserController_LoadUserContext(scoreloop_user_controller);
	if (loadUserContextResult == SC_OK)
	{
		state = LOAD_USER_CONTEXT;
	}
	else
	{
		// handle
		if (has_user_controller)
		{
			SC_UserController_Release(scoreloop_user_controller);
			has_user_controller = false;
		}
		state = DISCONNECTED;
		//printf("SC_UserController_LoadUserContext failed with code %i\n", loadUserContextResult); fflush(stdout);
		return;
	}
}

void OnlineService::userContextLoaded(SC_Error_t status)
{
	//printf("online userContextLoaded\n"); fflush(stdout);
	assert(state == LOAD_USER_CONTEXT);
	if (status != SC_OK)
	{
		if (has_user_controller)
		{
			SC_UserController_Release(scoreloop_user_controller);
			has_user_controller = false;
		}
		state = DISCONNECTED;
		printf("SC_UserController_LoadUserContext failed with code %i\n", status); fflush(stdout);
		return; // handle error
	}

	SC_Session_h session = SC_Client_GetSession(scoreloop_client);

	scoreloop_user = SC_Session_GetUser(session);
	has_user = true;

	SC_Context_h user_context = SC_User_GetContext(scoreloop_user);

	for (int i = 0; i < 5; i++)
	{
		const size_t keysize = 255;
		char key[keysize];
		snprintf(key, keysize, "save000%i.spe", i + 1);
		downloadSaveGame(key);
	}

	// fetch & decode autosave & resume file

	state = CONNECTED;
}

void OnlineService::disconnect()
{
	if (has_user_controller)
		SC_UserController_Release(scoreloop_user_controller);
	has_user_controller = false;
	state = DISCONNECTED;
}

void OnlineService::downloadSaveGame(const char *filename)
{
	//printf("online download %s\n", filename); fflush(stdout);
	const size_t pathsize = 255;
	char originalfilepath[pathsize];
	snprintf(originalfilepath, pathsize, "%s%s", get_save_filename_prefix(), filename);
	char encodedfilepath[pathsize];
	snprintf(encodedfilepath, pathsize, "%s%s.txt", get_save_filename_prefix(), filename);

	SC_Context_h user_context = SC_User_GetContext(scoreloop_user);

	SC_String_h save_data;

	//printf("getting user context entry %s... ", filename); fflush(stdout);
	SC_Error_t getContextResult = SC_Context_Get(user_context, filename, &save_data);

	if (getContextResult == SC_OK)
	{
		//printf("done\n"); fflush(stdout);

		//printf("writing %s... ", encodedfilepath); fflush(stdout);
		bFILE *file = open_file(encodedfilepath, "w");
		if(!file->open_failure())
		{
			//printf("done\n"); fflush(stdout);
			const char *data = SC_String_GetData(save_data);
			file->write(data, strlen(data));
		}
		else
		{
			//printf("failed\n"); fflush(stdout);
		}
		delete file;
		file = 0;

		//printf("decoding %s to %s... ", encodedfilepath, originalfilepath); fflush(stdout);
		int result = ascii85_decode(encodedfilepath, originalfilepath);
		//if (result == 0)
		//	printf("done\n");
		//else
		//	printf("failed\n");
		//fflush(stdout);
	}
	else if (getContextResult == SC_NOT_FOUND)
	{
		//printf("not found\n"); fflush(stdout);
	}
	else
	{
		//printf("failed\n"); fflush(stdout);
	}
}

void OnlineService::persistSaveGame(const char *filename)
{
	assert(has_user);
	assert(has_user_controller);
	assert(state == CONNECTED);

	const size_t pathsize = 255;
	char originalfilepath[pathsize];
	snprintf(originalfilepath, pathsize, "%s%s", get_save_filename_prefix(), filename);
	char encodedfilepath[pathsize];
	snprintf(encodedfilepath, pathsize, "%s%s.txt", get_save_filename_prefix(), filename);

	//printf("encoding %s to %s... ", originalfilepath, encodedfilepath); fflush(stdout);
	ascii85_encode(originalfilepath, encodedfilepath);
	//printf("done\n"); fflush(stdout);

	// read encoded save file
	//printf("reading %s... ", encodedfilepath); fflush(stdout);
	char *save_data = 0;
	bFILE *file = open_file(encodedfilepath, "r");
	if(!file->open_failure())
	{
		size_t filesize = file->file_size();
		save_data = new char[filesize + 1];
		file->read(save_data, filesize);
		save_data[filesize] = 0;
		//printf("read %i bytes\n", filesize); fflush(stdout);
	}
	else
	{
		//printf("failed\n"); fflush(stdout);
	}
	delete file;
	file = 0;

	// write encoded save file to context
	SC_String_h save_data_string;
	SC_Error_t newStringResult = SC_String_New(&save_data_string, save_data);

	SC_Context_h user_context = SC_User_GetContext(scoreloop_user);

	//printf("putting %s in user context... "); fflush(stdout);
	SC_Error_t putContextResult = SC_Context_Put(user_context, filename, save_data_string);
	//printf(putContextResult == SC_OK ? "done\n" : "failed\n"); fflush(stdout);

	//printf("setting user context... "); fflush(stdout);
	SC_Error_t setUserContextResult = SC_User_SetContext(scoreloop_user, user_context);
	//printf(setUserContextResult == SC_OK ? "done\n" : "failed\n"); fflush(stdout);

	//printf("updating user context...\n"); fflush(stdout);
	state = SUBMIT_USER_CONTEXT;
	SC_UserController_UpdateUserContext(scoreloop_user_controller);

	if (save_data)
	{
		delete save_data;
		save_data = 0;
	}
}

void OnlineService::userContextSubmitted(SC_Error_t status)
{
	assert(state == SUBMIT_USER_CONTEXT);

	//printf("updated user context: ");
	//printf(status == SC_OK ? "done\n" : "failed\n"); fflush(stdout);

	state = CONNECTED;
}

void OnlineService::update()
{
	SC_HandleCustomEvent(&scoreloop_init_data, SC_FALSE);
}

void OnlineService::updateUI(bps_event_t *event)
{
	//printf("updateUI\n"); fflush(stdout);
	SCUI_Client_HandleEvent(scoreloop_ui_client, event);
}

void onViewClosedCallback(void *cookie, SCUI_Result_t viewResult, const void *data)
{
	onlineservice->onViewClosed(cookie, viewResult, data);
}

void OnlineService::onViewClosed(void *cookie, SCUI_Result_t viewResult, const void *data)
{
	//printf("view closed\n"); fflush(stdout);
}

void OnlineService::showUI()
{
	//printf("showUI\n"); fflush(stdout);
	SCUI_Client_ShowFavoriteGamesView(scoreloop_ui_client, onViewClosedCallback, 0);
}

bool OnlineService::isConnected()
{
	return state > DISCONNECTED;
}

OnlineServiceState OnlineService::getState()
{
	return state;
}

OnlineService *onlineservice = new OnlineService();
