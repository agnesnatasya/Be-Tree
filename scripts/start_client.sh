BTREE_PATH='/home/ubuntu/aszekeres/Be-Tree'

DEBUG=all $BTREE_PATH/client/client --backingStoreDir $BTREE_PATH/store --benchmark benchmark-upserts --configFile $BTREE_PATH/config.txt --clientIP "10.172.209.107"
