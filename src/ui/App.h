#ifndef APP_H
#define APP_H

#include <Application.h>


class MainWindow;


extern const char* kAppSignature;


class App : public BApplication {
public:
								App();

	virtual	void				ArgvReceived(int32 argc, char** argv);
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				ReadyToRun();

private:
			MainWindow*			fWindow;
};


#endif	// APP_H
