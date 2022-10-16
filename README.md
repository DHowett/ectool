## ChromeOS ectool as an isolated repo

This repository was originally generated using [git-filter-repo](https://github.com/newren/git-filter-repo) with the following configuration:

```
glob:util/comm-*.cc
glob:util/comm-*.h
util/ectool.cc
util/ectool.h
util/ectool_keyscan.cc
util/ec_flash.cc
util/ec_flash.h
util/ec_panicinfo.cc
util/ec_panicinfo.h
util/ectool_i2c.cc
util/cros_ec_dev.h
util/misc_util.cc
util/misc_util.h
util/lock/file_lock.cc
util/lock/file_lock.h
util/lock/gec_lock.cc
util/lock/gec_lock.h
util/lock/ipc_lock.h
util/lock/locks.h

common/crc.c

util/==>src/
common/crc.c==>src/crc.cc

include/battery.h
include/board.h
include/BOARD_TASKFILE
include/chipset.h
include/common.h
include/compiler.h
include/compile_time_macros.h
include/config_chip.h
include/config.h
include/crc.h
include/ec_commands.h
include/ec_version.h
include/fuzz_config.h
include/gpio.inc
include/gpio_signal.h
include/host_command.h
include/i2c.h
include/keyboard_config.h
include/lightbar.h
include/lightbar_msg_list.h
include/module_id.h
include/panic.h
include/reset_flag_desc.inc
include/software_panic.h
include/task_filter.h
include/task_id.h
include/test_config.h
include/timer.h
include/usb_descriptor.h
include/usb_pd.h
include/usb_pd_tbt.h
include/usb_pd_tcpm.h
include/usb_pd_vdo.h
```
