const http = require('http')

const API = process.env.API_BASE || 'http://localhost:3000'
let passed = 0
let failed = 0

function get(path) {
  return new Promise((resolve, reject) => {
    http.get(`${API}${path}`, res => {
      let d = ''
      res.on('data', c => d += c)
      res.on('end', () => { try { resolve(JSON.parse(d)) } catch { resolve(d) } })
    }).on('error', reject)
  })
}

function req(method, path, body) {
  return new Promise((resolve, reject) => {
    const payload = body ? JSON.stringify(body) : null
    const opts = { method, headers: { 'Content-Type': 'application/json' } }
    if (payload) opts.headers['Content-Length'] = Buffer.byteLength(payload)
    const r = http.request(`${API}${path}`, opts, res => {
      let d = ''
      res.on('data', c => d += c)
      res.on('end', () => { try { resolve(JSON.parse(d)) } catch { resolve(d) } })
    })
    r.on('error', reject)
    if (payload) r.write(payload)
    r.end()
  })
}

async function test(name, fn) {
  try { await fn(); passed++; console.log(`  ✅ ${name}`) }
  catch (e) { failed++; console.log(`  ❌ ${name}: ${e.message}`) }
}

async function main() {
  console.log('🔗 AI Live Room - Frontend-Backend Integration Test\n')

  // -------- Verify backend health --------
  await test('Backend health check', async () => {
    const r = await get('/api/health')
    if (r.status !== 'ok') throw new Error('Backend not healthy')
  })

  // -------- Module 02: Frontend Gift System API contract --------
  console.log('\n--- Frontend Gift API Contract ---')

  await test('GET /api/gifts matches frontend GiftItem interface', async () => {
    const r = await get('/api/gifts')
    if (!r.success || !Array.isArray(r.data)) throw new Error('Invalid response')
    const item = r.data[0]
    const required = ['id', 'giftName', 'giftIcon', 'price', 'effectType', 'sortOrder']
    for (const key of required) {
      if (!(key in item)) throw new Error(`Missing field: ${key}`)
    }
    if (typeof item.price !== 'number') throw new Error('price should be number')
  })

  await test('POST /api/rooms/:id/gift response matches GiftEvent interface', async () => {
    const r = await req('POST', '/api/rooms/1/gift', { giftId: 1, count: 2, userId: 2, username: 'IntTest' })
    if (!r.success) throw new Error(r.error)
    const required = ['id', 'giftName', 'giftIcon', 'count', 'totalPrice', 'senderName', 'effectType', 'roomId']
    for (const key of required) {
      if (!(key in r.data)) throw new Error(`Missing field: ${key}`)
    }
    if (r.data.count !== 2) throw new Error(`count should be 2`)
  })

  await test('GET /api/rooms/:id/gift/rank returns GiftRankEntry[]', async () => {
    const r = await get('/api/rooms/1/gift/rank')
    if (!r.success || !Array.isArray(r.data)) throw new Error('Invalid response')
    if (r.data.length > 0) {
      const entry = r.data[0]
      const required = ['senderId', 'senderName', 'totalAmount']
      for (const key of required) {
        if (!(key in entry)) throw new Error(`Missing field: ${key}`)
      }
    }
  })

  // -------- Module 04: Room List + Video API contract --------
  console.log('\n--- Frontend Room List API Contract ---')

  await test('GET /api/rooms response matches frontend RoomCard interface', async () => {
    const r = await get('/api/rooms')
    if (!r.success || !r.data.rooms) throw new Error('Invalid response')
    const room = r.data.rooms[0]
    const required = ['roomId', 'roomName', 'hostId', 'hostName', 'title', 'coverImage', 'onlineCount', 'state']
    for (const key of required) {
      if (!(key in room)) throw new Error(`Missing field: ${key}`)
    }
  })

  await test('GET /api/rooms/:id response matches getRoomInfo contract', async () => {
    const r = await get('/api/rooms/1')
    if (!r.success) throw new Error(r.error)
    const required = ['roomId', 'roomName', 'hostId', 'hostName', 'title', 'coverImage', 'onlineCount', 'state']
    for (const key of required) {
      if (!(key in r.data)) throw new Error(`Missing field: ${key}`)
    }
  })

  // -------- Login flow --------
  console.log('\n--- Login Flow ---')

  let userToken = null
  await test('Login returns token + userInfo', async () => {
    const r = await req('POST', '/api/auth/login', { username: 'FrontEndTest' })
    if (!r.success || !r.data.token) throw new Error('Login failed')
    if (!r.data.userId || !r.data.username) throw new Error('Missing user fields')
    userToken = r.data.token
  })

  // -------- Danmaku API contract --------
  console.log('\n--- Danmaku API Contract ---')

  await test('GET danmaku response matches frontend expectations', async () => {
    const r = await get('/api/rooms/1/danmaku?limit=10')
    if (!r.success || !Array.isArray(r.data)) throw new Error('Invalid response')
    if (r.data.length > 0) {
      const msg = r.data[0]
      const required = ['id', 'username', 'content', 'color']
      for (const key of required) {
        if (!(key in msg)) throw new Error(`Missing field: ${key}`)
      }
    }
  })

  await test('POST danmaku returns complete message', async () => {
    const r = await req('POST', '/api/rooms/1/danmaku', {
      content: 'Integration test message',
      userId: 1,
      username: 'Tester',
      color: '#ff006e'
    })
    if (!r.success) throw new Error(r.error)
    if (r.data.content !== 'Integration test message') throw new Error('Content mismatch')
    if (!r.data.id) throw new Error('No id returned')
  })

  // -------- Error handling --------
  console.log('\n--- Error Handling ---')

  await test('404 on invalid route returns error object', async () => {
    const r = await get('/api/nonexistent')
    if (r.success === true) throw new Error('Should not succeed on invalid route')
  })

  await test('Invalid room returns error', async () => {
    const r = await get('/api/rooms/99999')
    if (r.success) throw new Error('Should fail for nonexistent room')
  })

  // -------- Summary --------
  console.log(`\n${'='.repeat(50)}`)
  console.log(`FRONTEND-BACKEND INTEGRATION: ${passed} passed, ${failed} failed, ${passed + failed} total`)
  if (failed === 0) console.log('✅ All API contracts validated - frontend and backend are fully compatible!')
  else console.log(`❌ ${failed} contract mismatches found`)
  console.log('='.repeat(50))
}

main().catch(e => console.error('Test runner error:', e))
