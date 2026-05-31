const autocannon = require('autocannon')
const http = require('http')

const TARGET = process.env.TARGET || 'http://localhost:3000'
const DURATION = parseInt(process.env.DURATION || '30')
const CONNECTIONS = parseInt(process.env.CONNECTIONS || '100')

console.log('========================================')
console.log('  Chatroom E2E Benchmark')
console.log('========================================')
console.log(`Target:     ${TARGET}`)
console.log(`Duration:   ${DURATION}s`)
console.log(`Connections: ${CONNECTIONS}`)
console.log('')

async function runBenchmark() {
  const result = await autocannon({
    url: TARGET,
    connections: CONNECTIONS,
    duration: DURATION,
    pipelining: 1,
    requests: [
      {
        method: 'GET',
        path: '/api/v1/health',
        headers: { 'content-type': 'application/json' }
      },
      {
        method: 'GET',
        path: '/api/v1/rooms',
        headers: { 'content-type': 'application/json' }
      },
      {
        method: 'GET',
        path: '/api/v1/rooms/1',
        headers: { 'content-type': 'application/json' }
      },
      {
        method: 'POST',
        path: '/api/v1/auth/login',
        headers: { 'content-type': 'application/json' },
        body: JSON.stringify({ username: 'test', password: 'test123456' })
      },
      {
        method: 'GET',
        path: '/api/v1/gifts',
        headers: { 'content-type': 'application/json' }
      }
    ]
  })

  console.log('\n=== Benchmark Report ===')
  console.log('─'.repeat(40))
  console.log(`Duration:      ${result.duration}s`)
  console.log(`Total req:     ${result.requests.total}`)
  console.log(`Avg req/s:     ${result.requests.average.toFixed(0)}`)
  console.log(`Max req/s:     ${result.requests.max}`)
  console.log(`Total errors:  ${result.errors}`)
  console.log(`Timeout:       ${result.timeouts}`)
  console.log(`Code non-2xx:  ${result.non2xx}`)
  console.log('')
  console.log('Latency:')
  console.log(`  min:         ${result.latency.min}ms`)
  console.log(`  p50:         ${result.latency.p50}ms`)
  console.log(`  p95:         ${result.latency.p95}ms`)
  console.log(`  p99:         ${result.latency.p99}ms`)
  console.log(`  max:         ${result.latency.max}ms`)
  console.log('')
  console.log('Throughput:')
  console.log(`  avg:         ${result.throughput.average.toFixed(0)} bytes/s`)
  console.log(`  total:       ${(result.throughput.total / 1024 / 1024).toFixed(2)} MB`)
  console.log('─'.repeat(40))

  const pass = result.errors === 0 && result.timeouts === 0
  console.log(`\nOverall: ${pass ? '✅ PASS' : '❌ FAIL'} (errors=${result.errors}, timeouts=${result.timeouts})`)

  return pass ? 0 : 1
}

if (require.main === module) {
  runBenchmark().then(exitCode => process.exit(exitCode))
}

module.exports = { runBenchmark }
