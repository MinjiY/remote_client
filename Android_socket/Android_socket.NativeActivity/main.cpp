/*
* Copyright (C) 2010 The Android Open Source Project
*
* Apache 라이선스 2.0 버전(이하 "라이선스")에 따라 라이선스가 부여됩니다.
* 라이선스를 준수하지 않으면 이 파일을 사용할 수 없습니다.
* 라이선스의 사본은
*
*      http://www.apache.org/licenses/LICENSE-2.0에서 얻을 수 있습니다.
*
* 적용 가능한 법률에 따라 필요하거나 서면으로 동의하지 않는 이상
* 라이선스에 따라 배포되는 소프트웨어는 "있는 그대로",
* 명시적 또는 묵시적이든 어떠한 유형의 보증이나 조건 없이 배포됩니다.
* 라이선스에 따른 특정 언어의 권한 및 제한에 대한 내용은
* 라이선스를 참조하세요.
*
*/

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

/**
* 저장된 상태 데이터입니다.
*/
struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
};

/**
* 앱에 대한 공유 상태입니다.
*/
struct engine {
	struct android_app* app;

	ASensorManager* sensorManager;
	const ASensor* accelerometerSensor;
	const ASensor* gyroscope_sensor;
	const ASensor* magnet_sensor;
	ASensorEventQueue* p_sensor_queue;

	int animating;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;
	struct saved_state state;
};

/**
* 현재 디스플레이에 대한 EGL 컨텍스트를 초기화합니다.
*/
static int engine_init_display(struct engine* engine) {
	// OpenGL ES 및 EGL 초기화

	/*
	* 여기에서 원하는 구성의 특성을 지정합니다.
	* 아래에서 화면 창과 호환되는 
	* 색상 구성 요소당 최소 8비트가 포함된 EGLConfig를 선택했습니다.
	*/
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};
	EGLint w, h, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* 여기에서 응용 프로그램이 원하는 구성을 선택합니다. 이 샘플에는
	* 기준에 일치하는 첫 번째 EGLConfig를 선택하는
	* 아주 간소화된 선택 과정이 포함되어 있습니다.*/
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID는 EGLConfig의 특성으로
	* ANativeWindow_setBuffersGeometry()에서 승인되도록 보장합니다.
	* EGLConfig를 선택하면 EGL_NATIVE_VISUAL_ID를 사용하여 
	* ANativeWindow 버퍼가 일치하도록 안전하게 다시 구성할 수 있습니다. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;
	engine->state.angle = 0;

	// GL 상태를 초기화합니다.
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);

	return 0;
}


/**
* 디스플레이의 현재 프레임입니다.
*/

#define SetRectF(p,sx,sy,ex,ey)  {p[0]=sx; p[1]=ey; p[2]=ex; p[3]=ey; p[4]=ex; p[5]=sy; p[6]=sx; p[7]=sy;}
#define INNER_X_MARGIN  0.02f
#define INNER_Y_MARGIN  0.01f


static void engine_draw_frame(struct engine* engine) {
	if (engine->display == NULL) {
		// 디스플레이가 없습니다.
		return;
	}

	// 화면을 색상으로 채웁니다.
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	int y, x;
	GLfloat pos[8];

	glEnableClientState(GL_VERTEX_ARRAY);
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 2; x++) {
			glColor4f(0.0, 0.3, 0.3, 1);
			SetRectF(pos, -1 + x * 1 + INNER_X_MARGIN, 1 - y*0.25 - INNER_Y_MARGIN, -1 + (x + 1) * 1 - INNER_X_MARGIN, 1 - (y + 1)*0.25 + INNER_Y_MARGIN);
			glVertexPointer(2, GL_FLOAT, 0, pos);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glColor4f(0.0, 0.05, 0.15, 1);
			SetRectF(pos, -1 + x * 1 + INNER_X_MARGIN + INNER_X_MARGIN, 1 - y*0.25 - INNER_Y_MARGIN - INNER_Y_MARGIN, -1 + (x + 1) * 1 - INNER_X_MARGIN - INNER_X_MARGIN, 1 - (y + 1)*0.25 + INNER_Y_MARGIN + INNER_Y_MARGIN);
			glVertexPointer(2, GL_FLOAT, 0, pos);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		}
	}
	glDisableClientState(GL_VERTEX_ARRAY);

	eglSwapBuffers(engine->display, engine->surface);
}

/**
* 현재 디스플레이에 연결된 EGL 컨텍스트를 분해합니다.
*/
static void engine_term_display(struct engine* engine) {
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}
	engine->animating = 0;
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

int gh_socket;

void SendFrameData(int ah_socket,
	unsigned char a_msg_id,
	const char *ap_data,
	unsigned short int a_data_size)
{
	int send_data_size = a_data_size + 4;
	char *p_send_data = new char[send_data_size];

	*p_send_data = 27;
	*(p_send_data + 1) = a_msg_id;
	*(unsigned short int *)(p_send_data + 2) = a_data_size;
	memcpy(p_send_data + 4, ap_data, a_data_size);
	send(ah_socket, p_send_data, send_data_size, 0);
	delete[] p_send_data;
}

/**
* 다음 입력 이벤트를 처리합니다.
*/
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {          // 터치에 관한 처리를 하겠다. (키에 대한 처리도 있음)
		if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN) {        // 터치 다운에 관한 처리 ( 업,무브에 대한 처리도 있음)
			engine->state.x = AMotionEvent_getX(event, 0);
			engine->state.y = AMotionEvent_getY(event, 0);

			int step_x = engine->width / 2;
			int step_y = engine->height / 8;

			int x = engine->state.x / step_x;
			int y = engine->state.y / step_y;

			int index = y * 2 + x;

			char temp[16];
			int length = sprintf(temp, "%d", index);
			SendFrameData(gh_socket, 1, temp, length + 1);
			SendFrameData(gh_socket, 2, (char *)&index, sizeof(int));
			return 1;
		}
	}
	return 0;
}

void StartSensor(engine* ap_engine)
{
	if (ap_engine->accelerometerSensor != NULL) {
		ASensorEventQueue_enableSensor(ap_engine->p_sensor_queue,
			ap_engine->accelerometerSensor);
		// 초당 60개의 이벤트를 가져올 수 있습니다.
		ASensorEventQueue_setEventRate(ap_engine->p_sensor_queue,
			ap_engine->accelerometerSensor, (1000L / 60) * 1000);
	}
	if (ap_engine->gyroscope_sensor != NULL) {
		ASensorEventQueue_enableSensor(ap_engine->p_sensor_queue,
			ap_engine->gyroscope_sensor);
		// 초당 60개의 이벤트를 가져올 수 있습니다.
		ASensorEventQueue_setEventRate(ap_engine->p_sensor_queue,
			ap_engine->gyroscope_sensor, (1000L / 60) * 1000);
	}
	if (ap_engine->magnet_sensor != NULL) {
		ASensorEventQueue_enableSensor(ap_engine->p_sensor_queue,
			ap_engine->magnet_sensor);
		// 초당 60개의 이벤트를 가져올 수 있습니다.
		ASensorEventQueue_setEventRate(ap_engine->p_sensor_queue,
			ap_engine->magnet_sensor, (1000L / 60) * 1000);
	}
}

void StopSensor(engine* ap_engine)
{
	if (ap_engine->accelerometerSensor != NULL) {
		ASensorEventQueue_disableSensor(ap_engine->p_sensor_queue, ap_engine->accelerometerSensor);
	}
	if (ap_engine->gyroscope_sensor != NULL) {
		ASensorEventQueue_disableSensor(ap_engine->p_sensor_queue, ap_engine->gyroscope_sensor);
	}
	if (ap_engine->magnet_sensor != NULL) {
		ASensorEventQueue_disableSensor(ap_engine->p_sensor_queue, ap_engine->magnet_sensor);
	}
}

/**
* 다음 주 명령을 처리합니다.
*/
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		// 시스템에서 현재 상태를 저장하도록 요청했습니다. 저장하세요.
		engine->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)engine->app->savedState) = engine->state;
		engine->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		// 창이 표시되어 준비를 마쳤습니다.
		if (engine->app->window != NULL) {
			engine_init_display(engine);
			engine_draw_frame(engine);
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// 창을 숨기거나 닫아 정리합니다.
		engine_term_display(engine);
		break;
	case APP_CMD_GAINED_FOCUS:
		// 앱에 포커스가 있으면 가속도계 모니터링을 시작합니다.
		StartSensor(engine);
		engine->animating = 1;
		break;
	case APP_CMD_LOST_FOCUS:
		// 앱에서 포커스가 사라지면 가속도계 모니터링이 중지됩니다.
		// 사용하지 않는 동안 배터리를 절약하기 위해 조치입니다.
		StopSensor(engine);
		// 애니메이션도 중지됩니다.
		engine->animating = 0;
		engine_draw_frame(engine);
		break;
	}
}

/**
* android_native_app_glue를 사용하는 네이티브 응용 프로그램의
* 주 진입점입니다. 자체 스레드에서 실행되며, 입력 이벤트를
* 받고 다른 작업을 수행하는 자체 이벤트 루프를 포함합니다.
*/




void android_main(struct android_app* state) {
	struct engine engine;
	TipsGyroscopeSensor gyro_sensor_manager;

	memset(&engine, 0, sizeof(engine));
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;

	// 가속도계 모니터링을 준비합니다.
	engine.sensorManager = ASensorManager_getInstance();
	engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager, ASENSOR_TYPE_ACCELEROMETER);
	engine.gyroscope_sensor = ASensorManager_getDefaultSensor(engine.sensorManager, ASENSOR_TYPE_GYROSCOPE);
	engine.magnet_sensor = ASensorManager_getDefaultSensor(engine.sensorManager, ASENSOR_TYPE_MAGNETIC_FIELD);
	engine.p_sensor_queue = ASensorManager_createEventQueue(engine.sensorManager, state->looper, LOOPER_ID_USER, NULL, NULL);

	if (state->savedState != NULL) {
		// 이전에 저장된 상태로 시작되며, 이 지점에서 복원됩니다.
		engine.state = *(struct saved_state*)state->savedState;
	}



	gh_socket = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in srv_addr;
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = inet_addr("172.30.1.33");
	srv_addr.sin_port = htons(25790);

	if (connect(gh_socket, (sockaddr *)&srv_addr, sizeof(srv_addr)) == 0) {
		SendFrameData(gh_socket, 1, "Hello~", 7);
	}

	engine.animating = 1;

	//수행할 작업을 대기하면서 루프를 실행합니다.

	while (1) {
		// 보류 중인 모든 이벤트를 읽습니다.
		int ident;
		int events;
		struct android_poll_source* source;
		float temp_data[3];
		// 애니메이션이 동작하지 않으면 이벤트 대기를 영구적으로 차단합니다.
		// 애니메이션이 동작하면 모든 이벤트를 읽을 때까지 루프를 실행한 다음
		// 계속해서 애니메이션의 다음 프레임을 그립니다.
		while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,	(void**)&source)) >= 0) {

			// 이 이벤트를 처리합니다.
			if (source != NULL) {source->process(state, source);}

			// 센서에 데이터가 있으면 바로 처리됩니다.
			if (ident == LOOPER_ID_USER) {
					ASensorEvent event;
					while (ASensorEventQueue_getEvents(engine.p_sensor_queue, &event, 1) > 0) {
						if (event.type == ASENSOR_TYPE_ACCELEROMETER) {
							gyro_sensor_manager.SetAccelerometerData(event.data);
						}
						else if (event.type == ASENSOR_TYPE_MAGNETIC_FIELD) {
							gyro_sensor_manager.SetMagneticFieldData(event.data);
						}
						else if (event.type == ASENSOR_TYPE_GYROSCOPE) {
							if (gyro_sensor_manager.SetGyroscopeData(event.data, event.timestamp, temp_data) == 1) {
								SendFrameData(gh_socket, 22, (const char*)temp_data, sizeof(float) * 3);
							}
						}
					}
			}

			// 종료 중인지 확인합니다.
			if (state->destroyRequested != 0) {
				engine_term_display(&engine);
				return;
			}
		}

		if (engine.animating) {
			// 이벤트를 종료한 후 다음 애니메이션 프레임을 그립니다.
			/* engine.state.angle += .01f;
			if (engine.state.angle > 1) {
				engine.state.angle = 0;
			}
			*/
			// 그리기는 화면 업데이트 속도의 제한을 받으므로
			// 여기에서는 타이밍을 계산할 필요가 없습니다.
			engine_draw_frame(&engine);
		}
	}
}
