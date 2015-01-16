CC := arm-none-linux-gnueabi-gcc
LINUX_DIR=/home/raymonxiu/workspace/exdroid/lichee/linux-3.3
CFLAGS := -I$(LINUX_DIR)
TARGET := app_test 

.PHONY: all clean


all: $(TARGET) 

app_test:app_test_ok.c 
	$(CC) $(CFLAGS) -static  $^  -o  $@

clean:
	rm -rf $(TARGET)  
