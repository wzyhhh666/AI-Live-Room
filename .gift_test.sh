#!/bin/bash
echo '=== Clean gift:input test ==='

echo 'Publishing gift:input with clean JSON...'
redis-cli -h redis PUBLISH gift:input '{"room_id":2,"sender_id":10,"sender_name":"gift_test","gift_id":5,"gift_name":"测试礼物","gift_count":2,"total_price":50.0,"effect_type":"rain","timestamp":1748621700000}'
RESULT=$?
echo "PUBLISH result: $RESULT"

sleep 2

echo '=== Log check ==='
grep -E 'publishGift|callback error|gift_record|listen loop' /tmp/srv-p1.log | tail -5
echo '=== Last 3 lines ==='
tail -3 /tmp/srv-p1.log

echo '=== MySQL check ==='
mysql -h mysql -u chatroom -pchatroom123 chatroom_db -e 'SELECT id, room_id, sender_id, sender_name, gift_name FROM gift_record ORDER BY id DESC LIMIT 3' 2>/dev/null
echo 'Done'
