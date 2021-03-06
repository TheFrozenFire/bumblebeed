/*
 * Copyright (c) 2011, The Bumblebee Project
 * Author: Joaquín Ignacio Aramendía samsagax@gmail.com
 * Author: Jaron Viëtor AKA "Thulinma" <jaron@vietors.com>
 *
 * This file is part of Bumblebee.
 *
 * Bumblebee is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Bumblebee is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bumblebee. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * bbconfig.c: Bumblebee configuration file handler
 */

#include "bbconfig.h"
#include "bblogger.h"
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>

struct bb_status_struct bb_status;
struct bb_config_struct bb_config;

struct bb_key_value {
  char key[BUFFER_SIZE];
  char value[BUFFER_SIZE];
};

/**
 * Takes a line and returns a key-value pair
 *
 * @param line String to be broken into a key-value pair
 */
static struct bb_key_value bb_get_key_value(const char* line) {
  struct bb_key_value kvpair;
  if (EOF == sscanf(line, "%[^=]=%[^\n]", kvpair.key, kvpair.value)) {
    int err_val = errno;
    bb_log(LOG_ERR, "Error parsing configuration file: %s\n", strerror(err_val));
  }
  return kvpair;
}

/**
 * Strips leading and trailing whitespaces from a string
 *
 * @param str String to be cleared of leading and trailing whitespaces
 */
static void strip_lead_trail_ws(char* str) {
  char *end;
  // Remove leading spaces
  while (isspace(*str)) {
    str++;
  }
  if (*str == 0) {
    return;
  }
  // Remove trailing spaces
  end = str + strlen(str) -1;
  while ((end > str) && (isspace(*end))) {
    end--;
  }
  // Add null terminator to end
  *(end+1) = 0;
}

/**
 *  Read the configuration file.
 *
 *  @return 0 on success.
 */
static int read_configuration( void ) {
  bb_log(LOG_DEBUG, "Reading configuration file: %s\n",bb_config.bb_conf_file);
  FILE *cf = fopen(bb_config.bb_conf_file, "r");
  if (cf == 0) { /* An error ocurred */
    bb_log(LOG_ERR, "Error in config file: %s\n", strerror(errno));
    bb_log(LOG_INFO, "Using default configuration\n");
    return 1;
  }
  char line[BUFFER_SIZE];
  while (fgets(line, sizeof line, cf) != NULL) {
    strip_lead_trail_ws(line);
    /* Ignore empty lines and comments */
    if ((line[0] != '#') && (line[0] != '\n')) {
      /* Parse configuration based on the run mode */
      struct bb_key_value kvp = bb_get_key_value(line);
      if (strncmp(kvp.key, "VGL_DISPLAY", 11) == 0) {
        snprintf(bb_config.x_display, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: x_display = %s\n", bb_config.x_display);
      } else if (strncmp(kvp.key, "STOP_SERVICE_ON_EXIT", 20) == 0) {
        bb_config.stop_on_exit = atoi(kvp.value);
        bb_log(LOG_DEBUG, "value set: stop_on_exit = %d\n", bb_config.stop_on_exit);
      } else if (strncmp(kvp.key, "X_CONFFILE", 10) == 0) {
        snprintf(bb_config.x_conf_file, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: x_conf_file = %s\n", bb_config.x_conf_file);
      } else if (strncmp(kvp.key, "VGL_COMPRESS", 12) == 0) {
        snprintf(bb_config.vgl_compress, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: vgl_compress = %s\n", bb_config.vgl_compress);
      } else if (strncmp(kvp.key, "ENABLE_POWER_MANAGEMENT", 23) == 0) {
        bb_config.pm_enabled = atoi(kvp.value);
        bb_log(LOG_DEBUG, "value set: pm_enabled = %d\n", bb_config.pm_enabled);
      } else if (strncmp(kvp.key, "FALLBACK_START", 15) == 0) {
        bb_config.fallback_start = atoi(kvp.value);
        bb_log(LOG_DEBUG, "value set: fallback_start = %d\n", bb_config.fallback_start);
      } else if (strncmp(kvp.key, "BUMBLEBEE_GROUP", 16) == 0) {
        snprintf(bb_config.gid_name, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: gid_name = %s\n", bb_config.gid_name);
      } else if (strncmp(kvp.key, "DRIVER", 7) == 0) {
        snprintf(bb_config.driver, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: driver = %s\n", bb_config.driver);
      } else if (strncmp(kvp.key, "NV_LIBRARY_PATH", 16) == 0) {
        snprintf(bb_config.ld_path, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: ld_path = %s\n", bb_config.ld_path);
      } else if (strncmp(kvp.key, "CARD_SHUTDOWN_STATE", 20) == 0) {
        bb_config.card_shutdown_state = atoi(kvp.value);
        bb_log(LOG_DEBUG, "value set: card_shutdown_state = %d\n", bb_config.card_shutdown_state);
      }
    }
  }
  fclose(cf);
  return 0;
}

/// Prints a single line, for use by print_usage, with alignment.
static void print_usage_line( char * opt, char * desc) {
  printf("  %-25s%s\n", opt, desc);
}

/**
 *  Print a little note on usage
 */
static void print_usage(int exit_val) {
  // Print help message and exit with exit code
  printf("%s version %s\n\n", bb_status.program_name, GITVERSION);
  if (strncmp(bb_status.program_name, "optirun", 8) == 0) {
    printf("Usage: %s [options] [--] [application to run] [application options]\n", bb_status.program_name);
  } else {
    printf("Usage: %s [options]\n", bb_status.program_name);
  }
  printf(" Options:\n");
  if (strncmp(bb_status.program_name, "optirun", 8) == 0) {
    //client-only options
    print_usage_line("-c [METHOD]", "Connection method to use for VirtualGL.");
    print_usage_line("--vgl-compress [METHOD]", "Connection method to use for VirtualGL.");
  } else {
    //server-only options
    print_usage_line("-D", "Run as daemon.");
    print_usage_line("--daemon", "Run as daemon.");
    print_usage_line("-x [PATH]", "xorg.conf file to use.");
    print_usage_line("--xconf [PATH]", "xorg.conf file to use.");
    print_usage_line("-g [GROUPNAME]", "Name of group to change to.");
    print_usage_line("--group [GROUPNAME]", "Name of group to change to.");
  }
  //common options
  print_usage_line("-q", "Be quiet (sets verbosity to zero)");
  print_usage_line("--quiet", "Be quiet (sets verbosity to zero)");
  print_usage_line("--silent", "Be quiet (sets verbosity to zero)");
  print_usage_line("-v", "Be more verbose (can be used multiple times)");
  print_usage_line("--verbose", "Be more verbose (can be used multiple times)");
  print_usage_line("-d [DISPLAY NAME]", "X display number to use.");
  print_usage_line("--display [DISPLAY NAME]", "X display number to use.");
  print_usage_line("-C [PATH]", "Configuration file to use.");
  print_usage_line("--config [PATH]", "Configuration file to use.");
  print_usage_line("-l [PATH]", "LD driver path to use.");
  print_usage_line("--ldpath [PATH]", "LD driver path to use.");
  print_usage_line("-s [PATH]", "Unix socket to use.");
  print_usage_line("--socket [PATH]", "Unix socket to use.");
  print_usage_line("-h", "Show this help screen.");
  print_usage_line("--help", "Show this help screen.");
  printf("\n");
  exit(exit_val);
}

/// Read the commandline parameters
static void read_cmdline_config( int argc, char ** argv ){
  /* Parse the options, set flags as necessary */
  int opt = 0;
  optind = 0;
  static const char *optString = "+Dqvx:d:s:g:l:c:C:Vh?";
  static const struct option longOpts[] = {
    {"daemon",0,0,'D'},
    {"quiet",0,0,'q'},
    {"silent",0,0,'q'},
    {"verbose",0,0,'v'},
    {"xconf",1,0,'x'},
    {"display",1,0,'d'},
    {"socket",1,0,'s'},
    {"group",1,0,'g'},
    {"ldpath",1,0,'l'},
    {"vgl-compress",1,0,'c'},
    {"config",1,0,'C'},
    {"help",1,0,'h'},
    {"silent",0,0,'q'},
    {"version",0,0,'V'},
    {0, 0, 0, 0}
  };
  while ((opt = getopt_long(argc, argv, optString, longOpts, 0)) != -1){
    switch (opt){
      case 'D'://daemonize
        bb_status.runmode = BB_RUN_DAEMON;
        break;
      case 'q'://quiet mode
        bb_status.verbosity = VERB_NONE;
        break;
      case 'v'://increase verbosity level by one
        bb_status.verbosity++;
        break;
      case 'x'://xorg.conf path
        snprintf(bb_config.x_conf_file, BUFFER_SIZE, "%s", optarg);
        break;
      case 'd'://X display number
        snprintf(bb_config.x_display, BUFFER_SIZE, "%s", optarg);
        break;
      case 's'://Unix socket to use
        snprintf(bb_config.socket_path, BUFFER_SIZE, "%s", optarg);
        break;
      case 'g'://group name to use
        snprintf(bb_config.gid_name, BUFFER_SIZE, "%s", optarg);
        break;
      case 'l'://LD driver path
        snprintf(bb_config.ld_path, BUFFER_SIZE, "%s", optarg);
        break;
      case 'c'://vglclient method
        snprintf(bb_config.vgl_compress, BUFFER_SIZE, "%s", optarg);
        break;
      case 'C'://config file
        snprintf(bb_config.bb_conf_file, BUFFER_SIZE, "%s", optarg);
        break;
      case 'V'://print version
        printf("Version: %s\n", GITVERSION);
        exit(EXIT_SUCCESS);
        break;
      default:
        print_usage(EXIT_FAILURE);
        break;
    }
  }
}

/// Read commandline parameters and config file.
/// Works by first setting compiled-in defaults,
/// then parsing commandline parameters,
/// then loading the config file,
/// finally again parsing commandline parameters.
void init_config( int argc, char ** argv ){
  /* set status */
  int i = 0;
  int lastslash = 0;
  //find the last slash in the program path
  while (argv[0][i] != 0){
    if (argv[0][i] == '/'){lastslash = i+1;}
    ++i;
  }
  //set program name
  strncpy(bb_status.program_name, argv[0]+lastslash, BUFFER_SIZE);
  bb_status.verbosity = VERB_WARN;
  bb_status.bb_socket = -1;
  bb_status.appcount = 0;
  bb_status.errors[0] = 0;//set first byte to NULL = empty string
  bb_status.x_pid = 0;
  if (strcmp(bb_status.program_name, "optirun") == 0){
    bb_status.runmode = BB_RUN_APP;
  }else{
    bb_status.runmode = BB_RUN_SERVER;
  }

  /* standard configuration */
  strncpy(bb_config.x_display, CONF_XDISP, BUFFER_SIZE);
  strncpy(bb_config.bb_conf_file, CONFIG_FILE, BUFFER_SIZE);
  strncpy(bb_config.ld_path, CONF_LDPATH, BUFFER_SIZE);
  strncpy(bb_config.socket_path, CONF_SOCKPATH, BUFFER_SIZE);
  strncpy(bb_config.gid_name, CONF_GID, BUFFER_SIZE);
  strncpy(bb_config.x_conf_file, CONF_XORG, BUFFER_SIZE);
  bb_config.pm_enabled = CONF_PMENABLE;
  bb_config.stop_on_exit = CONF_STOPONEXIT;
  bb_config.fallback_start = CONF_FALLBACKSTART;
  bb_config.card_shutdown_state = CONF_SHUTDOWNSTATE;
  strncpy(bb_config.vgl_compress, CONF_VGLCOMPRESS, BUFFER_SIZE);
  if (bb_config.driver[0] == 0){//only set driver if not autodetected
    strncpy(bb_config.driver, CONF_DRIVER, BUFFER_SIZE);
  }

  // parse commandline configuration (for config file, if changed)
  read_cmdline_config(argc, argv);
  // parse config file
  read_configuration();
  // parse commandline configuration again (so config file params are overwritten)
  read_cmdline_config(argc, argv);

  //check xorg.conf for %s, replace it by driver name
  char tmpstr[BUFFER_SIZE];
  while (strstr(bb_config.x_conf_file, "%s") != 0) {
    strncpy(tmpstr, bb_config.x_conf_file, BUFFER_SIZE);
    snprintf(bb_config.x_conf_file, BUFFER_SIZE, tmpstr, bb_config.driver);
  }

  //print configuration as debug messages
  bb_log(LOG_DEBUG, "Active configuration:\n");
  bb_log(LOG_DEBUG, " X display: %s\n", bb_config.x_display);
  bb_log(LOG_DEBUG, " xorg.conf file: %s\n", bb_config.x_conf_file);
  bb_log(LOG_DEBUG, " bumblebeed config file: %s\n", bb_config.bb_conf_file);
  bb_log(LOG_DEBUG, " LD_LIBRARY_PATH: %s\n", bb_config.ld_path);
  bb_log(LOG_DEBUG, " Socket path: %s\n", bb_config.socket_path);
  bb_log(LOG_DEBUG, " GID name: %s\n", bb_config.gid_name);
  bb_log(LOG_DEBUG, " Power management: %i\n", bb_config.pm_enabled);
  bb_log(LOG_DEBUG, " Stop X on exit: %i\n", bb_config.stop_on_exit);
  bb_log(LOG_DEBUG, " VGL Compression: %s\n", bb_config.vgl_compress);
  bb_log(LOG_DEBUG, " Driver: %s\n", bb_config.driver);
  bb_log(LOG_DEBUG, " Card shutdown state: %i\n", bb_config.card_shutdown_state);
}
