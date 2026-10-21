#ifndef APP_H
#define APP_H

#include <Application.h>


extern const char* kAppSignature;


class App : public BApplication {
public:
								App();

	virtual	void				ReadyToRun();
};


#endif	// APP_H
