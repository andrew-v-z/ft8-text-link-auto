extern int division;

//---------------------------------------------------------------------------
BOOL SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
	)  {
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if ( !LookupPrivilegeValue(
			NULL,            // lookup privilege on local system
			lpszPrivilege,   // privilege to lookup
			&luid ) )  {      // receives LUID of privilege
		printf("LookupPrivilegeValue error: %u\n", (int)GetLastError() );
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.

	if ( !AdjustTokenPrivileges(
		   hToken,
		   FALSE,
		   &tp,
		   sizeof(TOKEN_PRIVILEGES),
		   (PTOKEN_PRIVILEGES) NULL,
		   (PDWORD) NULL) )	{
		  printf("AdjustTokenPrivileges error: %u\n", (int)GetLastError() );
		  return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
		  printf("The token does not have the specified privilege. \n");
		  return FALSE;
	}

	return TRUE;
}

//-------------------------time_thread_t---------------------------------
class time_thread_t;
static time_thread_t *time_thread;
bool TThreadOff;

class time_thread_t : public TThread {
// Искомая последовательность
int p[5] = {739,2222,1110,2222,975};
// Шаблон разностей
int d[4], prev = 0, have_prev = 0, ok=1;
int buf[4];      // последние 4 разности

public:
  time_thread_t(bool CreateSuspended = false): TThread(CreateSuspended) {
	FreeOnTerminate = true;
	for (int i = 0; i < 4; i++)
		d[i] = p[i+1] - p[i];
  }

  void __fastcall beg(void) {
		Form1->ProgressBar1->Position = 0;
  }

  void __fastcall messNOT(void) {
		Application->MessageBox (L"Нет прав на установку времени.",	L"Ошибка", MB_OK );
		TThreadOff = true;
  }
  void __fastcall messOK(void) {
		Form1->SyncTimeMenuClick(this);
		Application->MessageBox (L"Синхронизация проведена успешно.",	L"Сообщение", MB_OK );
		TThreadOff = true;
  }

  void __fastcall ris(void) {
		Form1->InPB->Refresh();
  }

  void __fastcall end(void) {
		Form1->SyncTimeMenu->Checked = false;
		INvolume = 100;
		Form1->InPB->Refresh();
		Pa_Terminate();
		time_thread = NULL;
  }

  virtual void __fastcall Execute(void) {
	Synchronize(beg);

	struct tm tm_struct = { 0 };
	time_t mytime;

	int sample_rate = 12000, sec, min, count, zamed;
	monitor_t mon;
	monitor_config_t mon_cfg;

	int num_samples = 5 * sample_rate;
	float * signal = (float*)malloc(num_samples*sizeof(float));

	if (division==0) {
		gen_time_sync( 739, 1110, 975, signal);
	}

	audio_init();
	if (division==0) {
		if(audio_open(Form1->RealDevice(1), 1) != 0) TThreadOff = true;
	} else
		if(audio_open(Form1->RealDevice(0), 0) != 0) TThreadOff = true;

	// Compute FFT over the whole signal and store it
	mon_cfg.f_min = 200;
	mon_cfg.f_max = 3000;
	mon_cfg.sample_rate = sample_rate;
	mon_cfg.time_osr = kTime_osr;
	mon_cfg.freq_osr = kFreq_osr;
	mon_cfg.protocol = FTX_PROTOCOL_FT8;

	hashtable_init();

	monitor_init(&mon, &mon_cfg);

	int old_f = 0;
	count = 0;

	do {

		if (division==0) {
			// Wait for the end of hour
			while (true) {
				mytime = time(NULL);
				localtime_r(&mytime, &tm_struct);
				sec = tm_struct.tm_sec;
				min = tm_struct.tm_min;
				if (sec == 56) // && (min == 59))
					break;

				if (TThreadOff) 	break;
				Sleep(100);
			}
			for (int frame_pos = 0; frame_pos + 2048 <= num_samples; frame_pos += 2048) {
				if (TThreadOff)  break;

				audio_write(signal + frame_pos, 2048);
			}

//			TThreadOff = true;

		} else {

			// Process audio data in a monitor_time_process instance

			for (int frame_pos = 0; frame_pos + mon.block_size <= num_samples; frame_pos += mon.block_size) {
				mytime = time(NULL);
				localtime_r(&mytime, &tm_struct);
				sec = tm_struct.tm_sec + 60*tm_struct.tm_min + 3600*tm_struct.tm_hour;

				if (TThreadOff) break;

				audio_read(signal + frame_pos, mon.block_size);

				int f = monitor_time_process(&mon, signal + frame_pos);

				if ( (f>0) && (f!=old_f) && (abs(f - old_f)>10) ) {

					old_f = f;                             // 739,2222,1110,2222,975    ->   1483,-1112,1112,-1247
					Form1->Memo1->Lines->Add(IntToStr(f));

					if (!have_prev) {
						prev = f;
						have_prev = 1;
					}

					int delta = f - prev;
					prev = f;

					// сдвигаем буфер разностей
					if (count < 4) {
						buf[count++] = delta;
					} else {
						for (int i = 0; i < 3; i++)
							buf[i] = buf[i+1];
						buf[3] = delta;
					}
					if (count == 4) {
						// если накопили 4 разности — сравниваем
						ok = 0;
						for (int i = 0; i < 4; i++) {
							if (abs(buf[i] - d[i]) > 10) {
								ok = 1;
								break;
							}
						}
					}
					if (ok==0) {                     // можно ставить время

						BOOL isOK;
						HANDLE hToken;
						HANDLE hCurrentProcess;
						hCurrentProcess = GetCurrentProcess();
						isOK = OpenProcessToken(hCurrentProcess,TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
						if(!isOK) {
							Synchronize(messNOT);
						}
						if (SetPrivilege(hToken, SE_SYSTEMTIME_NAME, true)) {
							SYSTEMTIME NewTime;
							GetSystemTime(&NewTime);
//							if ((NewTime.wMinute > 58) || (NewTime.wMinute < 1)) {
//								if (NewTime.wMinute > 58)
//									NewTime.wHour++;
//								NewTime.wMinute = 0;
//							}
//							if ((NewTime.wSecond > 57) || (NewTime.wSecond < 3))
								NewTime.wSecond = 0;
							SetSystemTime(&NewTime);
							Synchronize(messOK);
						} else {
							Synchronize(messNOT);
						}
						break;
					}
				}                                    // if old

				if (zamed++>=2) {
					pMeterInfoIN->GetPeakValue(&alevelIN);
					INvolume = 100-alevelIN*100;
					Synchronize(ris);
					zamed = 0;
				}
			}                                        // for

			monitor_reset(&mon);

			if (TThreadOff) {
			  time_thread = NULL;
			  TThreadOff = false;
			  break;
			}
		} // if division
	} while (! TThreadOff);                          // do

	monitor_free(&mon);
	free(signal);

	Synchronize(end);
  }
};

//---------------------------------------------------------------------


