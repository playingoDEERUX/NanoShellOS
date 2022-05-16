/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

           Misc - timing module
******************************************/
#include <main.h>
#include <misc.h>
#include <memory.h>
#include <video.h>
#include <print.h>
#include <string.h>

#define CURRENT_YEAR 2022

// Real Time
#if 1
TimeStruct g_time;

TimeStruct* TmReadTime()
{
	return &g_time;
}

int TmCmosReadRegister(int reg) {
	WritePort(0x70,reg);
	return ReadPort(0x71);
}
void TmCmosWriteRegister(int reg, int data) {
	WritePort(0x70,reg);
	WritePort(0x71,data);
}

void TmGetTime (TimeStruct* pStruct) {
	do 
	{
		pStruct->statusA = TmCmosReadRegister(0x0A);
	} 
	while (pStruct->statusA & C_UPDATE_IN_PROGRESS_FLAG);
	pStruct->seconds = TmCmosReadRegister(0x00);
	pStruct->minutes = TmCmosReadRegister(0x02);
	pStruct->hours   = TmCmosReadRegister(0x04);
	pStruct->day     = TmCmosReadRegister(0x07);
	pStruct->month   = TmCmosReadRegister(0x08);
	pStruct->year    = TmCmosReadRegister(0x09);
	TimeStruct placeholder;
	do 
	{
		placeholder.seconds = pStruct->seconds; 
		placeholder.minutes = pStruct->minutes; 
		placeholder.hours   = pStruct->hours  ; 
		placeholder.day     = pStruct->day    ; 
		placeholder.month   = pStruct->month  ; 
		placeholder.year    = pStruct->year   ;
		
		do 
		{
			pStruct->statusA = TmCmosReadRegister(0x0A);
		} 
		while (pStruct->statusA & C_UPDATE_IN_PROGRESS_FLAG);
		
		pStruct->seconds = TmCmosReadRegister(0x00);
		pStruct->minutes = TmCmosReadRegister(0x02);
		pStruct->hours   = TmCmosReadRegister(0x04);
		pStruct->day     = TmCmosReadRegister(0x07);
		pStruct->month   = TmCmosReadRegister(0x08);
		pStruct->year    = TmCmosReadRegister(0x09);
		
	}
	while (!(
		placeholder.seconds == pStruct->seconds &&
		placeholder.minutes == pStruct->minutes &&
		placeholder.hours   == pStruct->hours   &&
		placeholder.day     == pStruct->day     &&
		placeholder.month   == pStruct->month   &&
		placeholder.year    == pStruct->year
	));
	
	pStruct->statusB = TmCmosReadRegister(0x0B);
	if (!(pStruct->statusB & 0x04))//BCD mode
	{
		pStruct->seconds = BCD_TO_BIN(pStruct->seconds);
		pStruct->minutes = BCD_TO_BIN(pStruct->minutes);
		pStruct->day = BCD_TO_BIN(pStruct->day);
		pStruct->month = BCD_TO_BIN(pStruct->month);
		pStruct->year = BCD_TO_BIN(pStruct->year);
		pStruct->hours= ( (pStruct->hours&0xF)+(((pStruct->hours&0x70)/16)*10))|(pStruct->hours&0x80);
	}
	
	//convert 12h to 24h
	if (!(pStruct->statusB & 0x02) && (pStruct->hours & 0x80))
		pStruct->hours = ((pStruct->hours & 0x7f)+12)%24;
	//calculate full year
	pStruct->year += 2000;
	if(pStruct->year<CURRENT_YEAR)pStruct->year+=100;
}

const char* g_monthNamesShort = 
	"Non\0Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec\0";

void TmPrintTime(TimeStruct* pStruct) {
	char hr[3],mn[3],sc[3];
	hr[0] = '0' + pStruct->hours/10;
	hr[1] = '0' + pStruct->hours%10;
	hr[2] = 0;
	mn[0] = '0' + pStruct->minutes/10;
	mn[1] = '0' + pStruct->minutes%10;
	mn[2] = 0;
	sc[0] = '0' + pStruct->seconds/10;
	sc[1] = '0' + pStruct->seconds%10;
	sc[2] = 0;
	
	LogMsg("Current time: %d %s %d  %s:%s:%s", 
		pStruct->day, &g_monthNamesShort[4*pStruct->month],
		pStruct->year,
		hr,mn,sc);
}
void TmPrintTimeFormatted(char* buffer, TimeStruct* pStruct) {
	char hr[3],mn[3],sc[3];
	hr[0] = '0' + pStruct->hours/10;
	hr[1] = '0' + pStruct->hours%10;
	hr[2] = 0;
	mn[0] = '0' + pStruct->minutes/10;
	mn[1] = '0' + pStruct->minutes%10;
	mn[2] = 0;
	sc[0] = '0' + pStruct->seconds/10;
	sc[1] = '0' + pStruct->seconds%10;
	sc[2] = 0;
	
	sprintf(
		buffer,
		"Current time: %d %s %d  %s:%s:%s", 
		pStruct->day, &g_monthNamesShort[4*pStruct->month],
		pStruct->year,
		hr,mn,sc);
}
#endif

// Run Time
#if 1
extern multiboot_info_t* g_pMultibootInfo;//main.c

int g_nRtcTicks = 0;
void GetTimeStampCounter(uint32_t* high, uint32_t* low)
{
	if (!high && !low) return; //! What's the point?
	int edx, eax;
	__asm__ volatile ("rdtsc":"=a"(eax),"=d"(edx));
	if (high) *high = edx;
	if (low ) *low  = eax;
}

uint64_t ReadTSC()
{
	uint32_t hi, lo;
	GetTimeStampCounter(&hi, &lo);
	return ((uint64_t)hi << 32LL) | (uint64_t)(lo);
}

//not accurate at all, use GetTickCount() instead.
int GetRtcBasedTickCount()
{
	return g_nRtcTicks * 1000 / RTC_TICKS_PER_SECOND;
}
int GetRawTickCount()
{
	return g_nRtcTicks;
}

extern uint64_t g_tscOneSecondAgo, g_tscTwoSecondsAgo;
extern int g_nSeconds;
int GetTickCount()
{
	uint64_t tscNow     = ReadTSC();
	uint64_t difference = tscNow - g_tscOneSecondAgo;             // the difference
	uint64_t tscPerSec  = g_tscOneSecondAgo - g_tscTwoSecondsAgo; // a full complete second has passed
	
	//SLogMsg("TSC per second: %q   Difference: %q   Tsc Now: %q", tscPerSec, difference, tscNow);
	
	int ms_count = 0;
	
	if (tscPerSec)
		ms_count = difference * 1000 / tscPerSec;
	
	return g_nSeconds * 1000 + ms_count;
}
#endif

// Kernel shutdown and restart
#if 1
__attribute__((noreturn))
void KeRestartSystem(void)
{
    uint8_t good = 0x02;
    while (good & 0x02)
        good = ReadPort(0x64);
    WritePort(0x64, 0xFE);
	
	// Still running.
	if (true)
	{
		// Try a triple fault instead.
		asm("mov $0, %eax\n\t"
			"mov %eax, %cr3\n");
		
		// If all else fails, declare defeat:
		KeStopSystem();
	}
}
#endif

// Random Number Generator
#if 1
int random_seed_thing[] = {
	-1244442060, -1485266709, 234119953, 
	-201730870, -1913591016, -1591339077, 
	2041649642, -886342341, 711555730, 
	40192431, 1517024340, -576781024, 
	-891284503, 1587790946, 1566147323, 
	2010441970, 1320486880, 66264233, 
	737611192, -1119832783, -865761768, 
	-420460857, 1682411333, -477284012, 
	-1116287873, -1211736837, 963092786,
	1730979436, 1505625485, 873340739, 
	1627747955, 1683315894, -1257054734, 
	1161594573, -1427794708, 648449375, 
	1030832345, -1089406083, 1545559989, 
	1407925421, -905406156, -1280911754, 
	-2001308758, 227385715, 520740534, 
	-1282033083, 1284654531, 1238828013, 
	148302191, -1656360560, -1139028545, 
	-704504193, 1367565640, -1213605929, 
	289692537, 2057687908, 684752482, 
	1301104665, 1933084912, 1134552268, 
	-1865611277, -757445071, -827902808, 
	-439546422, 1978388539, -997231600, 
	-1912768246, -922198660, 467538317, 
	-395047754, 146974967, 225791951, 
	-1521041638, 784853764, -586969394, 
	-465017921, 1258064203, 1500716775, 
	-250934705, -1117364667, -293202430, 
	-595719623, 376260241, -535874599, 
	-1939247052, 839033960, 1069014255, 
	-282157071, -22946408, -1178535916, 
	-537160644, -105327143, -316195415, 
	-571571219, 353753037, 1463803685, 
	1161010901, -404201378, 391199413, 
	405473472, 742889183, 452611239, 
	-1790628764, -1572764945, 917344354, 
	-1045176342, 1083902565, 1454280688, 
	-1995648000, 1037588263, 2145497403, 
	-1112202532, -1856081951, 938534213, 
	-1691760356, 2109781738, 1820109821, 
	1988028837, -2101599530, -926145485, 
	1852560250, 1917576092, -1377736232, 
	216878403, -1405586386, -544290439, 
	-382424683, -1477587186, -1488023165, 
	-1360589093, 25592369, 1695536505, 
	1821713859, -690132140, 1967963236, 
	1833534930, 74127397, -1987912089, 
	-586108586, -868646236, 1034085250, 
	527296915, 1954505506, 2083589286, 
	1608417972, 1461209721, 1669506543, 
	-140128717, 2089251682, -476684220, 
	1361481586, -543753628, 1954572638, 
	-1308070260, 1671930579, -922485963, 
	2047578920, -456758938, 1635678287, 
	-92864401, -457923115, 1647288373, 
	-852311725, 1513449752, 1503502780, 
	-98945231, -1537511900, -79182046, 
	635382674, 1144074041, -1850919743, 
	-2087056151, -1780458811, -582321540, 
	-1488230638, 2032974544, -1888292665, 
	205171085, 1986540608, -1362867570, 
	-358549401, -432582257, 2083465592, 
	440402956, 1274505014, -212283123, 
	865327044, -2051882447, -1521574964, 
	219506962, 2117666163, -45436631, 
	1947688981, -1094014751, -1712545352, 
	17866106, -1716024722, 1073364778, 
	625435084, -974600754, 505280162, 
	397970076, 643236003, 1046854828, 
	-944971290, -1297255861, -683254942, 
	721663995, 323535860, 1313747580, 
	-1802955617, -537520271, 1933763889, 
	1463929564, 268181342, -999074919, 
	-1399420127, 523583817, -844122414, 
	-1128805346, -1243791916, 593274684, 
	-82140258, -130641455, 2026672404, 
	841707801, -1597038831, 1654379730, 
	-1514952243, 400358679, 673293266, 
	839530185, 1371387967, 1469106392, 
	1646045314, 2112649705, -1727683438, 
	-555809424, 205081272, 748992652, 
	858137072, 1873953998, -884024544, 
	1336521232, -1480354168, 1238715379, 
	2009642630, 284719651, 1292482073, 
	1424572383, 971818364, -2069392610, 
	-1551855738, -1381069134, 1521291482, 
	-195336867, 
}, random_seed_thing2;
// basic garbage rand():
int GetRandom()
{
	//read the tsc:
	uint32_t hi, lo;
	GetTimeStampCounter(&hi, &lo);
	//combine the high and low numbers:
	
	hi ^= lo;
	
	//then mask it out so it wont look obvious:
	hi ^= 0xe671c4b4;
	
	//more masking
	hi ^= random_seed_thing[random_seed_thing2++];
	if (random_seed_thing2 >= (int)ARRAY_COUNT(random_seed_thing))
		random_seed_thing2 = 0;
	
	//then make it positive:
	hi &= 2147483647;
	
	//lastly, return.
	return (int) hi;
}
#endif

// CPUIDFeatureBits
#if 1
extern uint32_t g_cpuidLastLeaf;
extern char g_cpuidNameEBX[];
extern char g_cpuidBrandingInfo[];
extern CPUIDFeatureBits g_cpuidFeatureBits;

const char* GetCPUType()
{
	return g_cpuidNameEBX;
}
const char* GetCPUName()
{
	return g_cpuidBrandingInfo;
}
CPUIDFeatureBits GetCPUFeatureBits()
{
	return g_cpuidFeatureBits;
}
#endif

// Time Formatting
#if 1
//note: recommend an output buffer of at least 50 chars
void FormatTime(char* output, int formatType, int seconds)
{
	switch (formatType)
	{
		case FORMAT_TYPE_FIXED: {
			//sprintf(output, "SECONDS: %05d", seconds);
			int sec = seconds % 60;
			int min = seconds / 60 % 60;
			int hrs = seconds / 3600;
			sprintf(output, "%02d:%02d:%02d", hrs, min, sec);
			break;
		}
		case FORMAT_TYPE_VAR: {
			int sec = seconds % 60;
			int min = seconds / 60 % 60;
			int hrs = seconds / 3600;
			
			char buf[50];
			if (hrs)
			{
				sprintf(buf, "%d hour%s", hrs, hrs == 1 ? "" : "s");
				strcat (output, buf);
			}
			if (min)
			{
				if (hrs)
					sprintf(buf, ", %d min%s", min, min == 1 ? "" : "s");
				else
					sprintf(buf,   "%d min%s", min, min == 1 ? "" : "s");
				strcat (output, buf);
			}
			if (sec || !seconds)
			{
				if (min)
					sprintf(buf, ", %d sec%s", sec, sec == 1 ? "" : "s");
				else
					sprintf(buf,   "%d sec%s", sec, sec == 1 ? "" : "s");
				strcat (output, buf);
			}
			
			break;
		}
	}
}
#endif

// System Information
#if 1
void KePrintMemoryMapInfo()
{
	multiboot_info_t* mbi = g_pMultibootInfo;
	int len, addr;
	len = mbi->mmap_length, addr = mbi->mmap_addr;
	
	//turn this into a virt address:
	multiboot_memory_map_t* pMemoryMap;
	
	LogMsg("mmapAddr=%x mmapLen=%x", addr, len);
	addr += 0xC0000000;
	
	for (pMemoryMap = (multiboot_memory_map_t*)addr;
		 (unsigned long) pMemoryMap < addr + mbi->mmap_length;
		 pMemoryMap = (multiboot_memory_map_t*) ((unsigned long) pMemoryMap + pMemoryMap->size + sizeof(pMemoryMap->size)))
	{
		LogMsg("S:%x A:%x%x L:%x%x T:%x", pMemoryMap->size, 
			(unsigned)(pMemoryMap->addr >> 32), (unsigned)pMemoryMap->addr,
			(unsigned)(pMemoryMap->len  >> 32), (unsigned)pMemoryMap->len,
			pMemoryMap->type
		);
	}
}
void KePrintSystemInfoAdvanced()
{
	//oldstyle:
	/*
	LogMsg("Information about the system:");
	LogMsg("CPU Type:        %s", g_cpuidNameEBX);
	LogMsg("CPU Branding:    %s", g_cpuidBrandingInfo);
	LogMsg("Feature bits:    %x", *((int*)&g_cpuidFeatureBits));
	LogMsgNoCr("x86 Family   %d ", g_cpuidFeatureBits.m_familyID);
	LogMsgNoCr("Model %d ", g_cpuidFeatureBits.m_model);
	LogMsg("Stepping %d", g_cpuidFeatureBits.m_steppingID);
	LogMsg("g_cpuidLastLeaf: %d", g_cpuidLastLeaf);*/
	
	//nativeshell style:
	LogMsg("\x01\x0BNanoShell Operating System " VersionString);
	LogMsg("\x01\x0CVersion Number: %d", VersionNumber);
	
	LogMsg("\x01\x0F-------------------------------------------------------------------------------");
	LogMsg("\x01\x09[CPU] Name: %s", GetCPUName());
	LogMsg("\x01\x09[CPU] x86 Family %d Model %d Stepping %d.  Feature bits: %d",
			g_cpuidFeatureBits.m_familyID, g_cpuidFeatureBits.m_model, g_cpuidFeatureBits.m_steppingID);
	LogMsg("\x01\x0A[RAM] PageSize: 4K. Physical pages: %d. Total usable RAM: %d Kb", GetNumPhysPages(), GetNumPhysPages()*4);
	LogMsg("\x01\x0A[VID] Screen resolution: %dx%d.  Textmode size: %dx%d characters, of type %d.", GetScreenSizeX(), GetScreenSizeY(), 
																						g_debugConsole.width, g_debugConsole.height, g_debugConsole.type);
	LogMsg("\x01\x0F");
}

void KePrintSystemInfo()
{
	//neofetch style:
	int npp = GetNumPhysPages(), nfpp = GetNumFreePhysPages();
	
	char timingInfo[128];
	timingInfo[0] = 0;
	FormatTime(timingInfo, FORMAT_TYPE_VAR, GetTickCount() / 1000);
	LogMsg("\x01\x0E N    N       "      "\x01\x0C OS:       \x01\x0FNanoShell Operating System");
	LogMsg("\x01\x0E NN   N       "      "\x01\x0C Kernel:   \x01\x0F%s (%d)", VersionString, VersionNumber);
	LogMsg("\x01\x0E N N  N       "      "\x01\x0C Uptime:   \x01\x0F%s", timingInfo);
	LogMsg("\x01\x0E N  N N       "      "\x01\x0C CPU:      \x01\x0F%s", GetCPUName());
	LogMsg("\x01\x0E N   NN       "      "\x01\x0C CPU type: \x01\x0F%s", GetCPUType());
	LogMsg("\x01\x0E N    N\x01\x0D SSSS  \x01\x0C Memory:   \x01\x0F%d KB / %d KB", (npp-nfpp)*4, npp*4);
	LogMsg("\x01\x0D       S    S "      "\x01\x0C ");
	LogMsg("\x01\x0D       S      "      "\x01\x0C ");
	LogMsg("\x01\x0D        SSSS  "      "\x01\x0C ");
	LogMsg("\x01\x0D            S "      "\x01\x0C ");
	LogMsg("\x01\x0D       S    S "      "\x01\x0C ");
	LogMsg("\x01\x0D        SSSS  "      "\x01\x0C ");
	LogMsg("\x01\x0F");
}
#endif

// Mini clock
#if 1
const int g_mini_clock_cosa[]={    0, 105, 208, 309, 407, 500, 588, 669, 743, 809, 866, 914, 951, 978, 995,1000, 995, 978, 951, 914, 866, 809, 743, 669, 588, 500, 407, 309, 208, 105,   0,-105,-208,-309,-407,-500,-588,-669,-743,-809,-866,-914,-951,-978,-995,-1000,-995,-978,-951,-914,-866,-809,-743,-669,-588,-500,-407,-309,-208,-105 };
const int g_mini_clock_sina[]={-1000,-995,-978,-951,-914,-866,-809,-743,-669,-588,-500,-407,-309,-208,-105,   0, 105, 208, 309, 407, 500, 588, 669, 743, 809, 866, 914, 951, 978, 995,1000, 995, 978, 951, 914, 866, 809, 743, 669, 588, 500, 407, 309, 208, 105,    0,-105,-208,-309,-407,-500,-588,-669,-743,-809,-866,-914,-951,-978,-995 };

__attribute__((always_inline))
static inline void RenderThumbClockHand(int deg, int len, int cenX, int cenY, unsigned color)
{
	int begPointX = cenX,                                         begPointY = cenY;
	int endPointX = cenX + (g_mini_clock_cosa[deg] * len / 1000), endPointY = cenY + (g_mini_clock_sina[deg] * len / 1000);
	VidDrawLine (color, begPointX, begPointY, endPointX, endPointY);
}
void RenderThumbClock(int x, int y, int size)//=32
{
	if (size == 16) return;
	//render simple clock:
	TimeStruct* time = TmReadTime();
	
	int centerX = x + size/2, centerY = y + size/2;
	int diameter = size;
	int handMaxLength = (2 * diameter / 5);
	
	RenderThumbClockHand(time->hours % 12 * 5 + time->minutes / 12, 4 * handMaxLength / 9, centerX, centerY, 0xFF0000);
	RenderThumbClockHand(time->minutes,                             6 * handMaxLength / 9, centerX, centerY, 0x000000);
	RenderThumbClockHand(time->seconds,                             8 * handMaxLength / 9, centerX, centerY, 0x000000);
}

#endif


