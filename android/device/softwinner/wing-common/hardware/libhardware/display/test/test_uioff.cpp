#include <hardware/display.h>
#include <stdio.h>
#include <unistd.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayCommand.h>

using namespace android;

int main(int argc, char *argv[]){
	if(argc == 2){
		int left = atoi(argv[1]);
		int right = -left;
		SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SET3DLAYEROFFSET, 0, left, right);
	}else if(argc == 3){
	    int left = atoi(argv[1]);
		int right = atoi(argv[2]);
		SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SET3DLAYEROFFSET, 0, left, right);
	}else{
	    printf("Usage:\n");
		printf("    test_uioff [left_right_off]\n");
		printf("    or\n");
		printf("    test_uioff [left_off] [right_off]\n");
	}

	return 0;
}
