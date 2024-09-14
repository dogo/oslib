#include "oslib.h"

/*
	GRAPHICS
*/

int osl_intraInit = 0;
int osl_quit=0;
int osl_vblInterruptNumber=0;
int (*osl_powerCallback)(int, int, void*)=NULL;
int (*osl_exitCallback)(int, int, void*)=NULL;
OSL_CONTROLLER *osl_keys;
OSL_REMOTECONTROLLER *osl_remotekeys;

int osl_noFail = 0;
int osl_isinus[361];
int osl_icosinus[361];

void oslSetupFTrigo()		{
	int i;

	//Precalculate values for integer sine and cosine tables
	for (i=0;i<361;i++)			{
		osl_isinus[i] = (int)oslSin(i, 16384.0f);
		osl_icosinus[i] = (int)oslCos(i, 16384.0f);
	}
}

int oslCosi(int angle, int dist)		{
	angle = angle % 360;
	if (angle < 0)
		angle += 360;
	//Integer values are multiplicated by 16384
	return (osl_icosinus[angle] * dist) >> 14;
}

int oslSini(int angle, int dist)		{
	angle = angle % 360;
	if (angle < 0)
		angle += 360;
	return (osl_isinus[angle] * dist) >> 14;
}


int oslGetNextPower2(int val)			{
	int i;
	for (i=15;i>=0;i--)			{
		if (val & (1<<i))		{
			if (val & ~(1<<i))			{
				val = (1<<(i+1));
			}
			else
				val = (1<<i);
			break;
		}
	}
	return val;
}

//Aligne les donn�es aux align octets pr�s (align doit �tre une puissance de deux)
unsigned int oslAlignData(unsigned int data, int align)
{
	if (data & (align-1))
		data = (data & ~(align-1)) + align;
	return data;
}

//UTILISE UNIQUEMENT ICI, TROUVER UNE SOLUTION AVEC DMA HARDWARE, C'EST VRAIMENT PLUS RAPIDE!!!
void oslFasterMemset(u64 *dst, u64 *src, u32 length)			{
	u32 len = length>>6;
	u8 *dst2, *src2;
	while (len--)		{
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
	}
	len = (length & 63) >> 3;
	while (len--)
		*dst++ = *src;
	dst2 = (u8*)dst;
	src2 = (u8*)src;
	len = (length & 7);
	while (len--)
		*dst2++ = *src2++;
}

void oslWaitVSync()
{
	sceDisplayWaitVblankStart();
}

inline void oslFlushDataCache()		{
	sceKernelDcacheWritebackInvalidateAll();
}

int oslMeanBenchmarkTestEx(int startend, int slot)		{
	static int val[OSL_BENCH_SLOTS];
	static struct timeval start[OSL_BENCH_SLOTS];
	struct timeval end;
	static int curr_ms[OSL_BENCH_SLOTS], time[OSL_BENCH_SLOTS];

	if (startend == OSL_BENCH_INIT)			{
		val[slot]=0;
		time[slot]=0;
		curr_ms[slot]=0;
		gettimeofday(&start[slot],0);
	}
	else if (startend == OSL_BENCH_START)
		gettimeofday(&start[slot],0);
	else if (startend == OSL_BENCH_END)		{
		gettimeofday(&end,0);
		time[slot] += (end.tv_sec - start[slot].tv_sec) * 1000000;
		time[slot] += end.tv_usec - start[slot].tv_usec;
		val[slot]++;
		if (val[slot] >= OSL_BENCH_SAMPLES)
		{
			val[slot] = 0;
			curr_ms[slot] = time[slot] / OSL_BENCH_SAMPLES;
			time[slot] = 0;
		}
	}
	//Returns the last measure
	else if (startend == OSL_BENCH_GET_LAST)			{
		if (val[slot] != 0)
			return time[slot] / val[slot];
	}
	return curr_ms[slot];
}

int oslBenchmarkTestEx(int startend, int slot)		{
	static struct timeval start[OSL_BENCH_SLOTS];
	struct timeval end;
	static int time[OSL_BENCH_SLOTS];

	if (startend == OSL_BENCH_START)
		gettimeofday(&start[slot],0);
	else if (startend == OSL_BENCH_END)		{
		gettimeofday(&end,0);
		time[slot] = (end.tv_sec - start[slot].tv_sec) * 1000000;
		time[slot] += end.tv_usec - start[slot].tv_usec;
	}
	return time[slot];
}

static int osl_exitCbId = 0;	//<-- STAS: Store the OSL's standard exitcallback ID here

void oslQuit()
{								//<-- STAS: Totally rewritten routine
	osl_quit = 1;									// OSL mustQuit mark for another threads
	sceKernelDelayThread(500000);					// Let another threads to handle osl_quit marker

	if (!osl_exitCbId)
		sceKernelExitGame();						// We'll fall here if the OSL's exit callback was not set
	else {
		sceKernelNotifyCallback(osl_exitCbId, 0);	// Wake up our exit callback
		sceKernelSleepThreadCB();					// Sleep the thread forever...
	}							//<-- STAS END -->
}

#ifdef PSP
	/* Exit callback */
	int oslStandardExitCallback(int arg1, int arg2, void *common)
	{
		osl_quit = 1;
		if (osl_exitCallback)
			osl_exitCallback(arg1, arg2, common);
		sceKernelExitGame();		//<-- STAS: It's a good pratice to place sceKernelExitGame() here !
		return 0;
	}

	int oslStandardPowerCallback(int unknown, int pwrflags,void *common){
		if (osl_powerCallback)
			return osl_powerCallback(unknown, pwrflags, common);
		else
			return 0;
	}

	/* Callback thread */
	int oslCallbackThread(SceSize args, void *argp)
	{
		int cbid;

		cbid = sceKernelCreateCallback("exitCallback", oslStandardExitCallback, NULL);
		sceKernelRegisterExitCallback(cbid);
		osl_exitCbId = cbid;			//<-- STAS: store the exitcallback cbid for further use...
		cbid = sceKernelCreateCallback("powerCallback", oslStandardPowerCallback,NULL );
		scePowerRegisterCallback(0, cbid );

		sceKernelSleepThreadCB();

		return 0;
	}

	/* Sets up the callback thread and returns its thread id */
	int oslSetupCallbacks()
	{
		int thid = 0;

		thid = sceKernelCreateThread("update_thread", oslCallbackThread, 0x11, 0xFA0, 0, 0);
		if(thid >= 0)
		{
			sceKernelStartThread(thid, 0, 0);
		}

		return thid;
	}
#endif

//osl_currentFrameRate CANNOT be > 60!
volatile int osl_vblCount=0, osl_vblCountMultiple=0, osl_currentFrameRate, osl_vblCallCount=0, osl_skip=0, osl_nbSkippedFrames=0, osl_vblankCounterActive=1;
volatile int osl_vblShouldSwap = 0;

void oslVblankNextFrame()		{
	osl_vblCountMultiple += osl_currentFrameRate;
	//osl_currentFrameRate CANNOT be > 60! (else a while or a better method would be needed here)
	if (osl_vblCountMultiple >= 60)		{
		osl_vblCountMultiple -= 60;
		osl_vblCount++;
	}
}

void oslVblankInterrupt(int sub, void *parg)
{
	if (osl_vblankCounterActive)
		oslVblankNextFrame();

	//This is done so to avoid screen tearing when sound takes too long
	if (osl_vblShouldSwap)		{
		oslSwapBuffers();
		osl_vblShouldSwap = 0;
	}
}

int osl_maxFrameskip=0, osl_vsyncEnabled=0, osl_frameskip=0;

void oslInit(int flags)
{
	void *arg = 0;
	int i;

	osl_keys = &osl_pad;
	osl_remotekeys = &osl_remote;

	osl_quit = 0;
	osl_vblCount = 0;
	osl_vblCallCount = 0;
	osl_skip = 0;
	osl_nbSkippedFrames = 0;
	osl_maxFrameskip=5;
	osl_vsyncEnabled=4;
	osl_frameskip=0;
	osl_currentFrameRate = 60;

	//Configure l'autorepeat (il n'y en a pas au d�but)
	oslSetKeyAutorepeat(OSL_KEYMASK_UP|OSL_KEYMASK_RIGHT|OSL_KEYMASK_DOWN|OSL_KEYMASK_LEFT|OSL_KEYMASK_R|OSL_KEYMASK_L,0,0);

#ifdef PSP
	if (!(flags & OSL_IF_USEOWNCALLBACKS))
		oslSetupCallbacks();

	//No interrupt handler...
	if (!(flags & OSL_IF_NOVBLANKIRQ))		{
		sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT,osl_vblInterruptNumber,oslVblankInterrupt,arg);
		sceKernelEnableSubIntr(PSP_VBLANK_INT,osl_vblInterruptNumber);
	}
#endif
	for (i=0;i<OSL_BENCH_SLOTS;i++)
		oslMeanBenchmarkTestEx(OSL_BENCH_INIT, i);

	VirtualFileInit();

	//Initialise l'�mulation
#ifndef PSP
	emuInitGfx();
	emuStartDrawing();
	emuInitGL();
#endif
}

#define OSL_SYSTEM_BENCHMARK_ENABLED

void oslSysBenchmarkDisplay()			{
#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
	int ms4, ms5, ms6;
	ms4 = oslMeanBenchmarkTestEx(OSL_BENCH_GET, 4);
	ms5 = oslMeanBenchmarkTestEx(OSL_BENCH_GET, 5);
	ms6 = oslMeanBenchmarkTestEx(OSL_BENCH_GET, 6);
	oslSetTextColor(RGB(255,255,255));
	oslSetBkColor(RGBA(0,0,0,0x80));
	oslPrintf_xy(0,0, "%i.%03i + %i.%03i = %i.%03i", ms4/1000, ms4%1000, ms5/1000, ms5%1000, ms6/1000, ms6%1000);
#endif
}

/*
	Frameskip:
				0: pas de frameskip (normal)
				1: frameskip normal
				>1: d�pend de vsync, saute 1 frame sur X

	Max frameskip:
				>=1: Frameskip maximum autoris�
	VSync:
				0: pas de VSync
				1: vsync activ�e
				+4: si on rajoute 4, avec un frameskip >1, synchronise au framerate souhait�, par ex. 2 -> 30 fps
				+0: sinon, fixe le frameskip minimum (ex. avec 2, le jeu tournera � 60 fps, 30 images par seconde)
				+8: synchro maximale (impression de triple buffering), sans vsync
				+16: pas de swapbuffers
	Exemples:
		//30 fps (pas de frameskip)
		oslSyncFrameEx(2,0,0);
		//30 fps, jeu � 60, frameskip max de 2, c'est-�-dire pas plus d'une frame skipp�e pour une affich�e
		oslSyncFrameEx(2,2,4);
		//synchronise � 60 fps
		oslSyncFrameEx(0,0,0);
*/

int oslSyncFrameEx(int frameskip, int max_frameskip, int vsync)
{
	#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
	int lastOslSkip;
	#endif

	int i, wasDrawing=0;
	if (osl_isDrawingStarted)
		oslEndDrawing(), wasDrawing=1;
	if (frameskip==0)			{
		#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
		oslMeanBenchmarkTestEx(OSL_BENCH_END, 4);
		oslMeanBenchmarkTestEx(OSL_BENCH_END, 6);
		#endif
		//We need to do this outside of the handler to be more precise
		osl_vblankCounterActive = 0;
		if ((vsync & 5) || (osl_vblCallCount+1 > osl_vblCount)) {	//<-- STAS: (vsync&5) is more adequate here
			do {
				oslWaitVSync();
				oslVblankNextFrame();
			} while (osl_vblCallCount+1 > osl_vblCount);			//<-- STAS: check the condition AFTER VSync() !
		}
		osl_vblCallCount = osl_vblCount;
		osl_vblankCounterActive = 1;								//<-- STAS: resume counter in the intr handler

		osl_skip = 0;
		#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
		oslMeanBenchmarkTestEx(OSL_BENCH_START, 4);
		oslMeanBenchmarkTestEx(OSL_BENCH_START, 6);
		#endif
		if (!(vsync & 16))
			oslSwapBuffers();
	}
	else	{
		osl_vblCallCount++;				//<-- STAS: osl_vblCallCount+=frameskip is not good here
										//          because it wouldn't change osl_vblCallCount%frameskip value !
		#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
		lastOslSkip = osl_skip;
		if (osl_skip)
			oslMeanBenchmarkTestEx(OSL_BENCH_END, 5);
		else			{
			oslMeanBenchmarkTestEx(OSL_BENCH_END, 4);
		}
		#endif
		//La r�f�rence quand vsync = 0
		i=((vsync&1) && !osl_skip && !(vsync&8))?1:0;
		//On est en retard?				  <-- STAS: add frameskip-1 here
		if ((osl_vblCount+i > osl_vblCallCount+frameskip-1 || (vsync&4 && osl_vblCallCount%frameskip)) && osl_nbSkippedFrames < max_frameskip-1)			{
			if (!osl_skip)		{		//La frame a �t� dessin�e -> on l'affiche
				#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
					oslMeanBenchmarkTestEx(OSL_BENCH_END, 6);
				#endif
				if (vsync&1)						//Activer pour avoir vraiment la VSync
					oslWaitVSync();
				if (!(vsync & 16))
					oslSwapBuffers();
				#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
					oslMeanBenchmarkTestEx(OSL_BENCH_START, 6);
				#endif
			}
			osl_nbSkippedFrames++;
			//Derni�re fois -> la prochaine c'est en-dessous
			if (frameskip > 1 && osl_vblCallCount%frameskip == frameskip - 1)
				osl_skip = 0;
			else
				osl_skip = 1;
		}
		else		{
			#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
				if (!osl_skip)
					oslMeanBenchmarkTestEx(OSL_BENCH_END, 6);
			#endif
			if (vsync&1 && !osl_skip)						//Activer pour avoir vraiment la VSync
				oslWaitVSync();
			if (!(vsync&4))						//<-- STAS: add frameskip-1 to osl_vblCallCount
				osl_vblCallCount+=frameskip-1;	//          in order to skip correct number of frames
			//We need to do this outside of the handler to be more precise
			osl_vblankCounterActive = 0;
			while (osl_vblCount < osl_vblCallCount + ((vsync & 8) ? (1 - osl_skip) : (0)))		{
				oslWaitVSync();
				oslVblankNextFrame();
			}
			osl_vblankCounterActive = 1;

			if (!osl_skip)			{
				if (!(vsync & 16))
					oslSwapBuffers();
			}
			#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
				if (!osl_skip)
					oslMeanBenchmarkTestEx(OSL_BENCH_START, 6);
			#endif
			//Trop de frameskip tue le frameskip

			osl_vblCallCount = osl_vblCount;
			osl_skip = 0;
			//Bidouille pour le frameskip fix� -> on repasse en-dessus
			if (vsync&4 && frameskip > 1 && osl_vblCallCount%frameskip == 0)
				osl_skip = 1;
		}
		#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
		if (osl_skip)
			oslMeanBenchmarkTestEx(OSL_BENCH_START, 5);
		else
			oslMeanBenchmarkTestEx(OSL_BENCH_START, 4);
		#endif
	}
	if (!osl_skip) osl_nbSkippedFrames=0;						//<-- STAS: reset the osl_nbSkippedFrames
	if (wasDrawing)
		oslStartDrawing();
	return osl_skip;
}

void oslEndFrame()		{
	oslAudioVSync();
}
