const db = require('./database')

async function getGiftList() {
  const pool = await db.getMysqlPool()

  try {
    const [gifts] = await pool.execute(
      'SELECT id, gift_name, gift_icon, price, effect_type, sort_order FROM gift_config ORDER BY sort_order ASC'
    )

    return {
      success: true,
      data: gifts.map(g => ({
        id: g.id,
        giftName: g.gift_name,
        giftIcon: g.gift_icon,
        price: parseFloat(g.price),
        effectType: g.effect_type,
        sortOrder: g.sort_order
      }))
    }
  } catch (error) {
    console.error('Get gift list error:', error)
    return { success: false, error: 'Failed to get gift list' }
  }
}

async function sendGift(roomId, giftId, count, senderId, senderName) {
  const pool = await db.getMysqlPool()
  const redis = await db.getRedisClient()

  try {
    if (!roomId || !giftId || !count || count <= 0 || !senderId || !senderName) {
      return { success: false, error: 'Missing required fields' }
    }

    const [gifts] = await pool.execute(
      'SELECT id, gift_name, gift_icon, price, effect_type FROM gift_config WHERE id = ?',
      [giftId]
    )

    if (gifts.length === 0) {
      return { success: false, error: 'Gift not found' }
    }

    const [rooms] = await pool.execute(
      'SELECT id, host_id FROM rooms WHERE id = ?',
      [roomId]
    )

    if (rooms.length === 0) {
      return { success: false, error: 'Room not found' }
    }

    const gift = gifts[0]
    const room = rooms[0]
    const totalPrice = parseFloat(gift.price) * count

    const [result] = await pool.execute(
      `INSERT INTO gift_record (room_id, sender_id, receiver_id, gift_id, gift_count, total_price)
       VALUES (?, ?, ?, ?, ?, ?)`,
      [roomId, senderId, room.host_id, giftId, count, totalPrice]
    )

    await redis.zincrby(`rank:room:${roomId}:gift`, totalPrice, `${senderId}:${senderName}`)

    const data = {
      id: result.insertId,
      giftName: gift.gift_name,
      giftIcon: gift.gift_icon,
      count,
      totalPrice,
      senderName,
      effectType: gift.effect_type,
      roomId: parseInt(roomId),
      timestamp: Date.now()
    }

    return { success: true, data }
  } catch (error) {
    console.error('Send gift error:', error)
    return { success: false, error: 'Failed to send gift' }
  }
}

async function getGiftRank(roomId, limit = 20) {
  try {
    const redis = await db.getRedisClient()
    const rankKey = `rank:room:${roomId}:gift`
    const entries = await redis.zrevrange(rankKey, 0, limit - 1, 'WITHSCORES')
    // #region debug-point B:redis-result
    fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"B",location:"gift.js:92",msg:"[DEBUG] getGiftRank Redis entries count",data:{roomId,count:entries.length}})}).catch(()=>{});
    // #endregion

    if (entries.length > 0) {
      const data = []
      for (let i = 0; i < entries.length; i += 2) {
        const [senderId, senderName] = entries[i].split(':', 2)
        data.push({
          senderId: parseInt(senderId),
          senderName,
          totalAmount: parseFloat(entries[i + 1])
        })
      }
      // #region debug-point B:redis-result
      fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"B",location:"gift.js:105",msg:"[DEBUG] getGiftRank Redis result",data:{roomId,rankData:data}})}).catch(()=>{});
      // #endregion
      return { success: true, data }
    }
  } catch (error) {
    // #region debug-point B:redis-error
    fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"B",location:"gift.js:109",msg:"[DEBUG] getGiftRank Redis ERROR",data:{roomId,error:error.message}})}).catch(()=>{});
    // #endregion
  }

  try {
    const pool = await db.getMysqlPool()
    const [rows] = await pool.execute(
      `SELECT sender_id, sender_name, SUM(total_price) as total_amount
       FROM gift_record WHERE room_id = ? GROUP BY sender_id, sender_name
       ORDER BY total_amount DESC LIMIT ?`,
      [roomId, limit]
    )
    // #region debug-point B:mysql-rows
    fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"B",location:"gift.js:121",msg:"[DEBUG] getGiftRank MySQL fallback rows",data:{roomId,rowsLength:rows.length}})}).catch(()=>{});
    // #endregion
    const data = rows.map(r => ({
      senderId: r.sender_id,
      senderName: r.sender_name || 'Anonymous',
      totalAmount: parseFloat(r.total_amount)
    }))
    return { success: true, data }
  } catch (mysqlError) {
    // #region debug-point B:mysql-error
    fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"B",location:"gift.js:131",msg:"[DEBUG] getGiftRank MySQL ERROR",data:{roomId,error:mysqlError.message}})}).catch(()=>{});
    // #endregion
    return { success: true, data: [] }
  }
}

module.exports = {
  getGiftList,
  sendGift,
  getGiftRank
}
