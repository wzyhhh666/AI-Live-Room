const db = require('./database')
const crypto = require('crypto')

const STREAM_HOST = process.env.STREAM_HOST || 'localhost'
const STREAM_BASE = STREAM_HOST === 'localhost'
  ? { hls: `http://localhost:8088`, flv: `http://localhost:8080`, rtmp: `rtmp://localhost:1935` }
  : { hls: `https://${STREAM_HOST}`, flv: `https://${STREAM_HOST}`, rtmp: `rtmp://${STREAM_HOST}:1935` }

function generateStreamKey(roomId) {
  const shortUuid = crypto.randomUUID().replace(/-/g, '').substring(0, 8)
  const timestamp = Date.now().toString(16)
  return `room_${roomId}_${shortUuid}_${timestamp}`
}

async function startStream(roomId) {
  const pool = await db.getMysqlPool()

  try {
    const [rooms] = await pool.execute('SELECT state FROM rooms WHERE id = ?', [roomId])
    if (rooms.length === 0) {
      return { success: false, error: 'Room not found' }
    }

    const streamKey = generateStreamKey(roomId)

    await pool.execute('UPDATE rooms SET state = ?, online_count = 0 WHERE id = ?', ['living', roomId])
    await pool.execute(
      'INSERT INTO stream_records (room_id, stream_key, protocol, started_at) VALUES (?, ?, ?, NOW())',
      [roomId, streamKey, 'rtmp']
    )

    return {
      success: true,
      data: {
        streamKey,
        rtmpUrl: `${STREAM_BASE.rtmp}/live/${streamKey}`,
        hlsUrl: `${STREAM_BASE.hls}/hls/${streamKey}.m3u8`,
        flvUrl: `${STREAM_BASE.flv}/live/${streamKey}.flv`
      }
    }
  } catch (error) {
    console.error('streamService.startStream error:', error)
    return { success: false, error: error.message }
  }
}

async function stopStream(roomId) {
  const pool = await db.getMysqlPool()

  try {
    const [rooms] = await pool.execute('SELECT state FROM rooms WHERE id = ?', [roomId])
    if (rooms.length === 0) {
      return { success: false, error: 'Room not found' }
    }

    await pool.execute('UPDATE rooms SET state = ? WHERE id = ?', ['idle', roomId])
    await pool.execute(
      `UPDATE stream_records SET ended_at = NOW(), duration_sec = TIMESTAMPDIFF(SECOND, started_at, NOW())
       WHERE room_id = ? AND ended_at IS NULL ORDER BY started_at DESC LIMIT 1`,
      [roomId]
    )

    return { success: true }
  } catch (error) {
    console.error('streamService.stopStream error:', error)
    return { success: false, error: error.message }
  }
}

async function handlePublish(roomId, streamKey) {
  const pool = await db.getMysqlPool()

  try {
    await pool.execute('UPDATE rooms SET state = ?, online_count = 0 WHERE id = ?', ['living', roomId])

    const [result] = await pool.execute(
      'INSERT INTO stream_records (room_id, stream_key, protocol, started_at) VALUES (?, ?, ?, NOW())',
      [roomId, streamKey, 'rtmp']
    )

    console.log(`streamService: room ${roomId} publish started, recordId=${result.insertId}`)

    return {
      success: true,
      data: { recordId: result.insertId }
    }
  } catch (error) {
    console.error('streamService.handlePublish error:', error)
    return { success: false, error: error.message }
  }
}

async function handleUnpublish(roomId) {
  const pool = await db.getMysqlPool()

  try {
    await pool.execute(
      `UPDATE stream_records SET ended_at = NOW(), duration_sec = TIMESTAMPDIFF(SECOND, started_at, NOW())
       WHERE room_id = ? AND ended_at IS NULL ORDER BY started_at DESC LIMIT 1`,
      [roomId]
    )

    const [activeStreams] = await pool.execute(
      'SELECT COUNT(*) as cnt FROM stream_records WHERE room_id = ? AND ended_at IS NULL',
      [roomId]
    )

    if (activeStreams[0].cnt === 0) {
      await pool.execute('UPDATE rooms SET state = ? WHERE id = ?', ['idle', roomId])
    }

    console.log(`streamService: room ${roomId} publish ended, activeStreams=${activeStreams[0].cnt}`)

    return { success: true }
  } catch (error) {
    console.error('streamService.handleUnpublish error:', error)
    return { success: false, error: error.message }
  }
}

async function getActiveStreamKey(roomId) {
  const pool = await db.getMysqlPool()
  const [records] = await pool.execute(
    'SELECT stream_key FROM stream_records WHERE room_id = ? AND ended_at IS NULL ORDER BY started_at DESC LIMIT 1',
    [roomId]
  )
  return records.length > 0 ? records[0].stream_key : null
}

async function getStreamInfo(roomId) {
  const pool = await db.getMysqlPool()

  try {
    const [rooms] = await pool.execute('SELECT state FROM rooms WHERE id = ?', [roomId])

    if (rooms.length === 0) {
      return { success: false, error: 'Room not found' }
    }

    const isLiving = rooms[0].state === 'living'
    let streamUrls = null

    if (isLiving) {
      const streamKey = await getActiveStreamKey(roomId)
      if (streamKey) {
        streamUrls = {
          hls: `${STREAM_BASE.hls}/hls/${streamKey}.m3u8`,
          flv: `${STREAM_BASE.flv}/live/${streamKey}.flv`
        }
      }
    }

    return {
      success: true,
      data: {
        isLiving,
        streamUrls
      }
    }
  } catch (error) {
    console.error('streamService.getStreamInfo error:', error)
    return { success: false, error: error.message }
  }
}

async function getLatestStream(roomId) {
  const pool = await db.getMysqlPool()

  try {
    const [records] = await pool.execute(
      'SELECT * FROM stream_records WHERE room_id = ? ORDER BY started_at DESC LIMIT 1',
      [roomId]
    )

    return {
      success: true,
      data: records.length > 0 ? records[0] : null
    }
  } catch (error) {
    console.error('streamService.getLatestStream error:', error)
    return { success: false, error: error.message }
  }
}

module.exports = {
  startStream,
  stopStream,
  handlePublish,
  handleUnpublish,
  getStreamInfo,
  getLatestStream,
  getActiveStreamKey
}
