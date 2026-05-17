import { createApp } from 'vue'
import { createPinia } from 'pinia'
import App from './App.vue'
import router from './router'
import './style.css'

const app = createApp(App)

const pinia = createPinia()
app.use(pinia)

app.use(router)

app.config.errorHandler = (err, instance, info) => {
  console.error('Global error:', err)
  console.error('Error info:', info)
}

app.mount('#app')

const { useUserStore } = await import('./stores/user')
const userStore = useUserStore()
userStore.initFromStorage()

console.log('%c🚀 AI Live Room Desktop Client v1.0', 'color: #00ff41; font-size: 20px; font-weight: bold;')
console.log('%cCyberpunk Edition • Vue 3 + TypeScript • Real-time API', 'color: #8338ec; font-size: 12px;')
console.log('%c🔌 Connected to Backend Server', 'color: #00d4ff; font-size: 11px;')
