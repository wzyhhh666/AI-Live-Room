#!/bin/bash
echo '========================================'
echo ' Phase 2 P1 - Full End-to-End Verification'
echo '========================================'
echo ''

echo '=== [1/4] danmaku:input → C++ → danmaku:output ==='
PUBLISH_RESULT=$(redis-cli -h redis PUBLISH danmaku:input '{"room_id":1,"user_id":2,"username":"e2e","content":"P1_Test_Pass","color":"#00ff41","timestamp":1748621667000}')
echo "PUBLISH result: $PUBLISH_RESULT (expected: 1+)"
sleep 1
if grep -q 'DanmakuService: processed' /tmp/srv-p1.log; then
  echo '✅ danmaku:input received by C++'
else
  echo '❌ danmaku:input NOT received'
fi
if grep -q 'publishDanmaku room=1 user=2 content=P1_Test_Pass' /tmp/srv-p1.log; then
  echo '✅ danmaku:output published to Redis'
else
  echo '❌ danmaku:output NOT published'
fi

echo ''
echo '=== [2/4] gift:input → C++ → MySQL → gift:output ==='
PUBLISH_RESULT=$(redis-cli -h redis PUBLISH gift:input '{"room_id":1,"sender_id":3,"sender_name":"gifter","gift_id":5,"gift_name":"Rocket","gift_count":1,"total_price":99.9,"effect_type":"rocket","receiver_id":1,"timestamp":1748621668000}')
echo "PUBLISH result: $PUBLISH_RESULT (expected: 1+)"
sleep 1
if grep -q 'processing gift from user=gifter' /tmp/srv-p1.log; then
  echo '✅ gift:input received by C++'
else
  echo '❌ gift:input NOT received'
fi

echo ''
echo '=== [3/4] MySQL Persistence Check ==='
MYSQL_RESULT=$(mysql -h mysql -u chatroom -pchatroom123 chatroom_db -N -e 'SELECT record_id, room_id, sender_name, gift_name, gift_count, effect_type FROM gift_record ORDER BY record_id DESC LIMIT 1' 2>/dev/null)
if [ -n "$MYSQL_RESULT" ]; then
  echo "✅ MySQL has gift record: $MYSQL_RESULT"
else
  echo '❌ MySQL gift_record empty'
fi

echo ''
echo '=== [4/4] Log Summary (C++ Server) ==='
grep -E 'publishDanmaku|publishGift|processing gift|DanmakuService|callback error' /tmp/srv-p1.log | tail -8

echo ''
echo '========================================'
echo ' Verification Complete'
echo '========================================'
