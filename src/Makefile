CC 								:= gcc
DEV_FLAGS 						:= -Wall -Wno-discarded-qualifiers
CFLAGS 							:= $(DEV_FLAGS) -std=gnu99 -shared -fPIC
LIBS 							:= -ldl
SRC_FOLDER 						:= ./core/src/
SRC_STRUCT_FOLDER 				:= ./core/src/structs/
OUT_FOLDER 						:= ../bin/
OUT_ARCHIVE_FOLDER				:= ../bin/archive/
LIBYAML_BINARY_PATH				:= ../third_party/bin/libyaml/libyaml.a
GLOBAL_VARIABLES_PATH			:= $(SRC_STRUCT_FOLDER)GlobalVariables.h

MODULES 						:= SharedLibraries Utils HoneBookThread HoneyamlParsing
STRUCT_MODULES 					:= HoneywireBook HoneyWire HoneyWireSharedObjectModel SupportedTechnology
ARCHIVE_DEPENDENCIES			:= $(addsuffix .a, $(addprefix $(OUT_ARCHIVE_FOLDER), $(MODULES)))
STRUCT_ARCHIVE_DEPENDENCIES 	:= $(addsuffix .a, $(addprefix $(OUT_ARCHIVE_FOLDER), $(STRUCT_MODULES)))

DEFAULT_PATH 					:= ./default/src/
DEFAULT_FILES 					:= SharedLibraries_Default
DEFAULT_DEPENDENCIES			:= $(addsuffix .a, $(addprefix $(OUT_ARCHIVE_FOLDER), $(DEFAULT_FILES)))

DEV_PATH 						:= ./dev/src/
DEV_FILES 						:= DevUtils SharedLibraries_Dev
DEV_DEPENDENCIES				:= $(addsuffix .a, $(addprefix $(OUT_ARCHIVE_FOLDER), $(DEV_FILES)))

default: deceptionFramework
deceptionFramework: $(OUT_FOLDER)deception.so

$(OUT_FOLDER)deception.so: \
						$(SRC_FOLDER)Main.c \
						$(SRC_STRUCT_FOLDER)GlobalVariables.c \
						$(ARCHIVE_DEPENDENCIES) \
						$(STRUCT_ARCHIVE_DEPENDENCIES) \
						$(DEFAULT_DEPENDENCIES) \
						$(DEV_DEPENDENCIES)
	$(CC) $(CFLAGS) -o $(OUT_FOLDER)mount/deception.so \
		$(SRC_FOLDER)Main.c \
		$(ARCHIVE_DEPENDENCIES) \
		$(DEFAULT_DEPENDENCIES) \
		$(DEV_DEPENDENCIES) \
		$(STRUCT_ARCHIVE_DEPENDENCIES) \
		$(LIBYAML_BINARY_PATH) \
		$(LIBS)

# archives of $(ARCHIVE)
$(filter %,$(ARCHIVE_DEPENDENCIES)): $(OUT_ARCHIVE_FOLDER)%.a: $(SRC_FOLDER)%.c $(SRC_FOLDER)%.h $(GLOBAL_VARIABLES_PATH)
	$(CC) $(CFLAGS) -o $@ -c \
 		$<

# archives of $(STRUCT_ARCHIVE)
$(filter %,$(STRUCT_ARCHIVE_DEPENDENCIES)): $(OUT_ARCHIVE_FOLDER)%.a: $(SRC_STRUCT_FOLDER)%.c $(SRC_STRUCT_FOLDER)%.h $(GLOBAL_VARIABLES_PATH)
	$(CC) $(CFLAGS) -o $@ -c \
 		$<

# archives for default folder
$(filter %,$(DEFAULT_DEPENDENCIES)): $(OUT_ARCHIVE_FOLDER)%.a: $(DEFAULT_PATH)%.c $(DEFAULT_PATH)%.h $(GLOBAL_VARIABLES_PATH)
	$(CC) $(CFLAGS) -o $@ -c \
 		$<

# archives for dev folder
$(filter %,$(DEV_DEPENDENCIES)): $(OUT_ARCHIVE_FOLDER)%.a: $(DEV_PATH)%.c $(DEV_PATH)%.h $(GLOBAL_VARIABLES_PATH)
	$(CC) $(CFLAGS) -o $@ -c \
 		$<

submodule-libyaml-make:
	cd ../third_party/lib/libyaml && \
	ls && \
	./bootstrap && \
	./configure CFLAGS="-DYAML_DECLARE_STATIC" --enable-static --disable-shared && \
	make && \
	echo 'The compiled binary is available at: "../third_party/lib/libyaml/src/.libs"'

link:
	printf 'Please manually execute the following code:\n  export LD_PRELOAD=/workspaces/ld-preload-deception/bin/deception.so\n'
unlink:
	printf 'Please manually execute the following code:\n  unset LD_PRELOAD\n'

clang:
	find . -type f -iname "*.c" -o -iname "*.h" | xargs clang-format -i

clean:
	rm $(OUT_ARCHIVE_FOLDER)*
	rm ../bin/deception.so
