#!/bin/bash
echo '=== Phase 2 P1 End-to-End Verification ==='
echo ''

echo '--- Test 1: danmaku:input ---'
redis-cli -h redis PUBLISH danmaku:input '{"room_id":1,"user_id":2,"username":"e2e","content":"P1_Danmaku_Test","color":"#00ff41","timestamp":1748621667000}'
sleep 1
echo 'C++ log after danmaku:'
tail -3 /tmp/srv-p1.log 2>/dev/null
echo ''

echo '--- Test 2: gift:input ---'
redis-cli -h redis PUBLISH gift:input '{"room_id":1,"sender_id":3,"sender_name":"e2e_gifter","gift_id":1,"gift_name":"Rocket","gift_count":1,"total_price":99.9,"effect_type":"rocket","timestamp":1748621668000}'
sleep 1
echo 'C++ log after gift:'
tail -3 /tmp/srv-p1.log 2>/dev/null
echo ''

echo '--- Test 3: MySQL gift_record ---'
mysql -h mysql -u chatroom -pchatroom123 chatroom_db -e 'SELECT id, room_id, sender_id, gift_name, gift_count FROM gift_record ORDER BY id DESC LIMIT 1' 2>/dev/null
echo ''

echo '--- Test 4: MySQL danmaku ---'
mysql -h mysql -u chatroom -pchatroom123 chatroom_db -e 'SELECT id, room_id, user_id, username, content FROM danmaku ORDER BY id DESC LIMIT 1' 2>/dev/null
echo ''

echo '=== VERIFICATION COMPLETE ==='
