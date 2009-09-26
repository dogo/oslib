#include <pspkernel.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("Ad hoc sample", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12*1024);

OSL_IMAGE *bkg = NULL;
OSL_FONT *pgfFont = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Init OSLib:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initOSLib(){
    oslInit(0);
    oslInitGfx(OSL_PF_8888, 1);
    oslInitAudio();
    oslSetQuitOnLoadFailure(1);
    oslSetKeyAutorepeatInit(40);
    oslSetKeyAutorepeatInterval(10);
}


//Prints some info:
void printInfo()
{
	u8 *macAddress = oslAdhocGetMacAddress();
	oslIntraFontSetStyle(pgfFont, 1.0, RGBA(255,255,255,255), RGBA(0,0,0,0), INTRAFONT_ALIGN_LEFT);
	oslDrawStringf(10, 10, "Current state: %s", oslAdhocGetState() == ADHOC_INIT ? "OK" : "KO");
	oslDrawStringf(10, 25, "Your MAC address: %02X:%02X:%02X:%02X:%02X:%02X", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
}

//The client is connected and can send data:
void clientConnected(struct remotePsp *aPsp)
{
    int skip = 0;
	char mess[100] = "Hello distant World !!!";

    while(!osl_quit){
        if (!skip){
            oslStartDrawing();

            oslDrawImageXY(bkg, 0, 0);

			printInfo();
			oslDrawStringf(10, 40, "Press O to send a message to %s", aPsp->name);
            oslDrawString(150, 250, "Press X to quit");

            oslEndDrawing();
        }
        oslEndFrame();
        skip = oslSyncFrame();

        oslReadKeys();
		if (osl_keys->released.cross)
		{
			oslQuit();
		}
		else if (osl_keys->released.circle)
		{
			oslAdhocSendData(aPsp, mess, strlen( mess));
		}
	}
}

//Connects to a psp:
void doClient()
{
    int skip = 0;
	int quit = 0;
	int i = 0;
	int current = 0;

	int init = oslAdhocInit("ULUS99999");
	if (init)
	{
		char message[100] = "";
		sprintf(message, "adhocInit error: %i", init);
		oslMessageBox(message, "Debug",  oslMake3Buttons(OSL_KEY_CROSS, OSL_MB_OK , 0, 0, 0, 0));
		return;
	}

    while(!osl_quit && !quit){
        if (!skip){
            oslStartDrawing();

            oslDrawImageXY(bkg, 0, 0);

			printInfo();
			if (oslAdhocGetRemotePspCount())
				oslDrawString(10, 40, "Press X to request a connection");
			oslDrawString(10, 55, "Press O to abort");
			for (i=0; i < oslAdhocGetRemotePspCount(); i++)
			{
				if (i == current)
				    oslIntraFontSetStyle(pgfFont, 1.0, RGBA(100,100,100,255), RGBA(0,0,0,0), INTRAFONT_ALIGN_LEFT);
				else
				    oslIntraFontSetStyle(pgfFont, 1.0, RGBA(255,255,255,255), RGBA(0,0,0,0), INTRAFONT_ALIGN_LEFT);
				oslDrawString(10, 100 + 15 * i, oslAdhocGetPspByIndex(i)->name);
			}
            oslEndDrawing();
        }
        oslEndFrame();
        skip = oslSyncFrame();

		oslReadKeys();
		if (osl_keys->released.down)
		{
			if (++current >= oslAdhocGetRemotePspCount())
				current = 0;
		}
		else if (osl_keys->released.up)
		{
			if (--current < 0)
				current = oslAdhocGetRemotePspCount() - 1;
		}
		else if (osl_keys->released.cross && oslAdhocGetRemotePspCount())
		{
			//Request a connection:
			int ret = oslAdhocRequestConnection(oslAdhocGetPspByIndex(current), 30, NULL);
			if (ret == OSL_ADHOC_ACCEPTED || ret == OSL_ADHOC_ESTABLISHED)
			{
				clientConnected(oslAdhocGetPspByIndex(current));
			}
		}
		else if (osl_keys->released.circle)
		{
			quit = 1;
		}
	}
	oslAdhocTerm();
}


//The server accepted the connection and it's ready to receive data:
void serverConnected(struct remotePsp *aPsp)
{
    int skip = 0;
	char buffer[100] = "";
	int dataLength = 0;

    while(!osl_quit){
		if (dataLength <= 0)
			dataLength = oslAdhocReceiveData(aPsp, buffer, 100);

        if (!skip){
            oslStartDrawing();

            oslDrawImageXY(bkg, 0, 0);

			printInfo();

			if (dataLength <= 0)
			{
				oslDrawStringf(10, 40, "Waiting for data from %s", aPsp->name);
			}
			else
			{
				oslDrawStringf(10, 40, "Data received from %s:", aPsp->name);
				oslDrawStringf(10, 55, buffer);
				oslDrawString(10, 70, "Press O to receive more data");
			}
			oslDrawString(150, 250, "Press X to quit");
            oslEndDrawing();
        }
        oslEndFrame();
        skip = oslSyncFrame();

        oslReadKeys();
		if (osl_keys->released.cross)
		{
			oslQuit();
		}
		else if (osl_keys->released.circle && dataLength > 0)
		{
			dataLength = 0;
			memset(buffer, sizeof(buffer), 0);
		}
	}
}


//Waits for a connection from a psp:
void doServer()
{
    int skip = 0;
	int quit = 0;
	char message[100] = "";
	int dialog = OSL_DIALOG_NONE;
	int connected = 0;

	int init = oslAdhocInit("ULUS99999");
	if (init)
	{
		snprintf(message, sizeof(message), "adhocInit error: %i", init);
		oslMessageBox(message, "Debug",  oslMake3Buttons(OSL_KEY_CROSS, OSL_MB_OK , 0, 0, 0, 0));
		return;
	}

    while(!osl_quit && !quit){
		struct remotePsp *reqPsp = oslAdhocGetConnectionRequest();

        if (!skip){
            oslStartDrawing();

            oslDrawImageXY(bkg, 0, 0);

			printInfo();
			oslDrawString(10, 40, "Press O to abort");
			if (reqPsp == NULL)
			{
				oslDrawString(10, 100, "Waiting for a connection request...");
			}
			else
			{
				snprintf(message, sizeof(message), "Accept request from psp : %s", reqPsp->name);
				oslDrawString(10, 100, message);
				if (oslGetDialogType() == OSL_DIALOG_NONE)
					oslInitMessageDialog(message, 1);
			}

            dialog = oslGetDialogType();
            if (dialog){
                oslDrawDialog();
                if (oslGetDialogStatus() == PSP_UTILITY_DIALOG_NONE){
                    if (dialog == OSL_DIALOG_MESSAGE){
                        int button = oslGetDialogButtonPressed();
                        if (button == PSP_UTILITY_MSGDIALOG_RESULT_YES)
						{
							oslAdhocAcceptConnection(reqPsp);
							connected = 1;
						}
                        else if (button == PSP_UTILITY_MSGDIALOG_RESULT_NO)
						{
							oslAdhocRejectConnection(reqPsp);
						}
                    }
                    oslEndDialog();
                }
			}

            oslEndDrawing();
        }
        oslEndFrame();
        skip = oslSyncFrame();

		if (connected)
			serverConnected(reqPsp);

		oslReadKeys();
		if (dialog == OSL_DIALOG_NONE)
		{
			if (osl_keys->released.circle)
			{
				quit = 1;
			}
		}
	}
	oslAdhocTerm();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(){
    int skip = 0;

    initOSLib();
    oslIntraFontInit(INTRAFONT_CACHE_MED);

    //Loads image:
    bkg = oslLoadImageFilePNG("bkg.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);

    //Load font:
    pgfFont = oslLoadFontFile("flash0:/font/ltn0.pgf");
    oslIntraFontSetStyle(pgfFont, 1.0, RGBA(255,255,255,255), RGBA(0,0,0,0), INTRAFONT_ALIGN_LEFT);
    oslSetFont(pgfFont);

    while(!osl_quit){
        if (!skip){
            oslStartDrawing();

            oslDrawImageXY(bkg, 0, 0);

			oslIntraFontSetStyle(pgfFont, 1.0, RGBA(255,255,255,255), RGBA(0,0,0,0), INTRAFONT_ALIGN_LEFT);
			if (oslIsWlanPowerOn())
			{
				oslDrawString(10, 10, "Press [] to act like a server");
				oslDrawString(10, 25, "Press O to act like a client");
			}
			else
			{
				oslDrawString(10, 10, "Please turn on the wlan switch");
			}
            oslDrawString(150, 250, "Press X to quit");

            oslEndDrawing();
        }
        oslEndFrame();
        skip = oslSyncFrame();

        oslReadKeys();
        if (osl_keys->released.circle && oslIsWlanPowerOn())
			doClient();
        else if (osl_keys->released.square && oslIsWlanPowerOn())
			doServer();
        else if (osl_keys->released.cross)
            oslQuit();
    }
    oslEndGfx();

    sceKernelExitGame();
    return 0;

}
