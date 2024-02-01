SOURCES		:= auto_tests/send_message_test.c \
		   auto_tests/auto_test_support.c \
		   testing/misc_tools.c \
		   $(wildcard tox*/*.c tox*/*/*.c) \
		   third_party/cmp/cmp.c

CFLAGS		:= $(shell pkg-config --cflags libsodium opus vpx)
LDFLAGS		:= $(shell pkg-config --libs libsodium opus vpx)
#CPPFLAGS	:= -DDISABLE_STATIC_ASSERT -DLOGGER_TO_STDERR -DMIN_LOGGER_LEVEL=LOGGER_LEVEL_TRACE

send_message_test: $(SOURCES)
	/work/dyibicc/out/lr/dyibicc $(CFLAGS) $+
