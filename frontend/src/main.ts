import { createApp } from 'vue'
import { createPinia } from 'pinia'
import App from './App.vue'
import router from './router'
import { useUserStore } from './stores/user'
import { useTheme } from './composables/useTheme'
import './style.css'

const app = createApp(App)

const pinia = createPinia()
app.use(pinia)

app.use(router)

app.config.errorHandler = (err, instance, info) => {
  console.error('Global error:', err)
  console.error('Error info:', info)
}

const userStore = useUserStore()
userStore.initFromStorage()

const { initFromStorage: initTheme } = useTheme()
initTheme()

app.mount('#app')

console.log('%c🚀 AI Live Room Desktop Client v1.0', 'color: #00ff41; font-size: 20px; font-weight: bold;')
console.log('%cCyberpunk Edition • Vue 3 + TypeScript • Real-time API', 'color: #8338ec; font-size: 12px;')
console.log('%c🔌 Connected to Backend Server', 'color: #00d4ff; font-size: 11px;')
