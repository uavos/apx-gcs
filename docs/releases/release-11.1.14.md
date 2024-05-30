# Bug Fixes
* Autoupdate crush bug
* [`11`](https://github.com/uavos/apx-gcs/issues/11) Access modifiers order fix
* [`44`](https://github.com/uavos/apx-gcs/issues/44) exchange creating symlink on .xpl plugin
* [`45`](https://github.com/uavos/apx-gcs/issues/45) add exception check
* [`45`](https://github.com/uavos/apx-gcs/issues/45) plugins blacklisted on segfault

# Comments

**fix:[`44`](https://github.com/uavos/apx-gcs/issues/44) exchange creating symlink on .xpl plugin**

with copying of it

**fix:[`45`](https://github.com/uavos/apx-gcs/issues/45) add exception check**

to give other plugins chance
to go out of blacklist

**fix:[`45`](https://github.com/uavos/apx-gcs/issues/45) plugins blacklisted on segfault**

replace try-catch with removing plugin
from blacklist while dependency is loading
