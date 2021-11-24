BTREE_PATH='/home/ubuntu/aishwaryag/Be-Tree'

DEBUG=all $BTREE_PATH/client/dfs-client --backingStoreDir $BTREE_PATH/store --benchmark benchmark-upserts --configFile $BTREE_PATH/config.txt --clientIP "10.172.209.107"
