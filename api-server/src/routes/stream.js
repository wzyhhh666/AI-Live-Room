const express = require('express')
const router = express.Router()
const streamService = require('../services/streamService')

const STREAM_HOST = process.env.STREAM_HOST || 'localhost'
const STREAM_BASE = STREAM_HOST === 'localhost'
  ? { hls: `http://localhost:8088`, flv: `http://localhost:8080` }
  : { hls: `https://${STREAM_HOST}`, flv: `https://${STREAM_HOST}` }

router.post('/callback', async (req, res) => {
  try {
    const { action, stream } = req.body

    if (!action || !stream) {
      return res.status(400).json({ code: -1, message: 'Missing action or stream' })
    }

    const roomId = parseInt(stream.replace('room_', ''), 10)
    if (isNaN(roomId) || roomId <= 0) {
      console.warn(`stream route: invalid stream name "${stream}", cannot extract roomId`)
      return res.status(400).json({ code: -1, message: 'Invalid stream name format' })
    }

    const io = req.app.get('io')
    const room = `room-${roomId}`

    if (action === 'on_publish') {
      const result = await streamService.handlePublish(roomId, stream)
      if (result.success && io) {
        io.to(room).emit('stream-started', {
          roomId,
          hlsUrl: `${STREAM_BASE.hls}/hls/${stream}.m3u8`,
          flvUrl: `${STREAM_BASE.flv}/live/${stream}.flv`,
          timestamp: Date.now()
        })
      }
      console.log(`stream route: room ${roomId} publish callback received, broadcasting stream-started`)
    } else if (action === 'on_unpublish') {
      const result = await streamService.handleUnpublish(roomId)
      if (result.success && io) {
        io.to(room).emit('stream-ended', {
          roomId,
          timestamp: Date.now()
        })
      }
      console.log(`stream route: room ${roomId} unpublish callback received, broadcasting stream-ended`)
    } else {
      console.warn(`stream route: unknown action "${action}"`)
      return res.status(400).json({ code: -1, message: 'Unknown action' })
    }

    res.json({ code: 0 })
  } catch (error) {
    console.error('stream route error:', error)
    res.status(200).json({ code: 0 })
  }
})

router.get('/rooms/:roomId/stream/info', async (req, res) => {
  try {
    const roomId = parseInt(req.params.roomId)
    if (isNaN(roomId)) {
      return res.status(400).json({ success: false, error: 'Invalid roomId' })
    }

    const result = await streamService.getStreamInfo(roomId)
    if (!result.success) {
      return res.status(404).json(result)
    }

    res.json(result)
  } catch (error) {
    console.error('stream route getStreamInfo error:', error)
    res.status(500).json({ success: false, error: error.message })
  }
})

router.post('/rooms/:roomId/stream/start', async (req, res) => {
  try {
    const roomId = parseInt(req.params.roomId)
    console.log(`[stream:start] RECEIVED roomId=${roomId}, origin=${req.get('origin')}`)
    if (isNaN(roomId)) {
      return res.status(400).json({ success: false, error: 'Invalid roomId' })
    }

    const result = await streamService.startStream(roomId)
    if (!result.success) {
      // #region debug-point B:start-fail
      (()=>{require('http').request('http://127.0.0.1:7778/event',{method:'POST',headers:{'Content-Type':'application/json'}},r=>{}).end(JSON.stringify({sessionId:'stream-viewer-no-video',runId:'pre',hypothesisId:'B',location:'stream.js:startStream',msg:'[DEBUG] stream/start FAILED',data:{roomId,error:result.error},ts:Date.now()}))})();
      // #endregion
      return res.status(400).json(result)
    }

    const io = req.app.get('io')
    if (io) {
      io.to(`room-${roomId}`).emit('stream-started', {
        roomId,
        hlsUrl: result.data.hlsUrl,
        flvUrl: result.data.flvUrl,
        timestamp: Date.now()
      })
    }

    // #region debug-point B:start-ok
    (()=>{require('http').request('http://127.0.0.1:7778/event',{method:'POST',headers:{'Content-Type':'application/json'}},r=>{}).end(JSON.stringify({sessionId:'stream-viewer-no-video',runId:'pre',hypothesisId:'B',location:'stream.js:startStream',msg:'[DEBUG] stream/start OK, emitted to room',data:{roomId,hlsUrl:result.data.hlsUrl,flvUrl:result.data.flvUrl,hasIo:!!io},ts:Date.now()}))})();
    // #endregion

    res.json(result)
  } catch (error) {
    console.error('stream route start error:', error)
    res.status(500).json({ success: false, error: error.message })
  }
})

router.post('/rooms/:roomId/stream/stop', async (req, res) => {
  try {
    const roomId = parseInt(req.params.roomId)
    if (isNaN(roomId)) {
      return res.status(400).json({ success: false, error: 'Invalid roomId' })
    }

    const result = await streamService.stopStream(roomId)
    if (!result.success) {
      return res.status(400).json(result)
    }

    const io = req.app.get('io')
    if (io) {
      io.to(`room-${roomId}`).emit('stream-ended', {
        roomId,
        timestamp: Date.now()
      })
    }

    res.json(result)
  } catch (error) {
    console.error('stream route stop error:', error)
    res.status(500).json({ success: false, error: error.message })
  }
})

module.exports = router
