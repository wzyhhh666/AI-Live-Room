#!/bin/bash
echo 'Publishing danmaku:input...'
redis-cli -h redis PUBLISH danmaku:input '{"room_id":1,"user_id":2,"username":"e2e","content":"P1_Danmaku_Test","color":"#00ff41","timestamp":1748621667000}'
sleep 1

echo 'Publishing gift:input...'
redis-cli -h redis PUBLISH gift:input '{"room_id":1,"sender_id":3,"sender_name":"e2e_gifter","gift_id":1,"gift_name":"Rocket","gift_count":1,"total_price":99.9,"effect_type":"rocket","timestamp":1748621668000}'
sleep 2

echo '=== Full log (gift+danmaku+error) ==='
grep -E 'DanmakuService|publishDanmaku|publishGift|gift_record|listen loop|error|callback' /tmp/srv-p1.log

echo '=== Last 5 lines ==='
tail -5 /tmp/srv-p1.log
