// What oslLoadSoundFile hands each extension to, and what loading an MP3 or AT3
// does before oslInitAudioME (the docs said it crashes; it runs in user mode).
#include <pspkernel.h>
#include <psputility.h>
#include <oslib/oslib.h>
#include <stdio.h>

PSP_MODULE_INFO("soundload", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static FILE *f;

// Loads through oslLoadSoundFile and checks it picked the same loader, streamed, as the direct call
static void check(const char *file, OSL_SOUND *(*direct)(const char *, int)) {
	OSL_SOUND *s = oslLoadSoundFile(file, OSL_FMT_STREAM);
	OSL_SOUND *d = direct ? direct(file, OSL_FMT_STREAM) : NULL;
	int ok = direct ? s && d && s->audioCallback == d->audioCallback && !s->isStreamed == !d->isStreamed : !s;
	fprintf(f, "oslLoadSoundFile(%-20s) -> %s %s\n", file, s ? "sound" : "NULL", ok ? "PASS" : "FAIL");
	if (s) oslDeleteSound(s);
	if (d) oslDeleteSound(d);
}

int main() {
	oslInit(0);
	oslInitAudio();
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0); // keep what was written if a load below crashes

	// A previous test may have left the codec module loaded
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
	fprintf(f, "before oslInitAudioME:\n");
	OSL_SOUND *s = oslLoadSoundFileMP3("../mp3/tone44.mp3", OSL_FMT_STREAM);
	fprintf(f, "oslLoadSoundFileMP3 -> %s\n", s ? "sound" : "NULL");
	if (s) oslDeleteSound(s);
	s = oslLoadSoundFileAT3("../at3/lp2.at3", OSL_FMT_STREAM);
	fprintf(f, "oslLoadSoundFileAT3 -> %s\n", s ? "sound" : "NULL");
	if (s) oslDeleteSound(s);

	oslInitAudioME(OSL_FMT_MP3 | OSL_FMT_AT3);
	fprintf(f, "after oslInitAudioME:\n");
	check("../wavload/tone44s.wav", oslLoadSoundFileWAV);
	check("tone.bgm", oslLoadSoundFileBGM);
	check("TONE.BGM", oslLoadSoundFileBGM);
	check("../mp3/tone44.mp3", oslLoadSoundFileMP3);
	check("../at3/lp2.at3", oslLoadSoundFileAT3);
	check("../at3/gen.sh", NULL); // unknown extension
	fprintf(f, "DONE\n");
	fclose(f);
	sceKernelExitGame();
	return 0;
}
