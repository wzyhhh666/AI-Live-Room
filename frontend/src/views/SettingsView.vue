<script setup lang="ts">
import { useRouter } from 'vue-router'
import { useUserStore } from '@/stores/user'
import { useTheme } from '@/composables/useTheme'

const router = useRouter()
const userStore = useUserStore()
const { theme, setTheme } = useTheme()

const themes: Array<{ value: string; label: string; icon: string }> = [
  { value: 'dark', label: 'Dark', icon: '🌙' },
  { value: 'light', label: 'Light', icon: '☀️' },
  { value: 'warm', label: 'Warm', icon: '🌿' },
]

function handleLogout() {
  userStore.logout()
  router.push('/')
}
</script>

<template>
  <div class="min-h-screen" style="background-color: var(--bg-primary)">
    <div class="max-w-2xl mx-auto px-6 py-12">
      <button
        @click="router.back()"
        class="flex items-center gap-2 mb-8 hover:opacity-80 transition-opacity"
        style="color: var(--text-secondary)"
      >
        <span>←</span>
        <span class="text-sm">Back</span>
      </button>

      <h1 class="text-3xl font-bold mb-10" style="font-family: 'Orbitron', sans-serif; color: var(--text-primary)">
        ⚙ Settings
      </h1>

      <div
        class="rounded-xl p-6 mb-6 backdrop-blur-sm"
        style="background-color: var(--bg-secondary); border: 1px solid var(--border-color)"
      >
        <h2 class="text-lg font-semibold mb-4" style="color: var(--text-primary)">Theme</h2>
        <div class="flex gap-3">
          <button
            v-for="t in themes"
            :key="t.value"
            @click="setTheme(t.value as any)"
            class="flex-1 py-3 rounded-lg text-sm font-medium transition-all"
            :class="theme === t.value
              ? 'ring-2 ring-pink-400 bg-pink-500/20 text-pink-300'
              : 'hover:bg-gray-700/30'"
            :style="{ border: `1px solid ${theme === t.value ? 'transparent' : 'var(--border-color)'}`, color: `var(--text-secondary)` }"
          >
            <span class="text-xl block mb-1">{{ t.icon }}</span>
            {{ t.label }}
          </button>
        </div>
      </div>

      <div
        class="rounded-xl p-6 mb-6 backdrop-blur-sm"
        style="background-color: var(--bg-secondary); border: 1px solid var(--border-color)"
      >
        <h2 class="text-lg font-semibold mb-4" style="color: var(--text-primary)">Danmaku Settings</h2>
        <div class="space-y-4">
          <div>
            <div class="flex justify-between text-sm mb-1" style="color: var(--text-secondary)">
              <span>Density</span>
              <span>50%</span>
            </div>
            <input type="range" class="w-full accent-pink-500" value="50" />
          </div>
          <div>
            <div class="flex justify-between text-sm mb-1" style="color: var(--text-secondary)">
              <span>Speed</span>
              <span>60%</span>
            </div>
            <input type="range" class="w-full accent-pink-500" value="60" />
          </div>
          <div>
            <div class="flex justify-between text-sm mb-1" style="color: var(--text-secondary)">
              <span>Opacity</span>
              <span>40%</span>
            </div>
            <input type="range" class="w-full accent-pink-500" value="40" />
          </div>
          <div>
            <div class="flex justify-between text-sm mb-1" style="color: var(--text-secondary)">
              <span>Font Size</span>
              <span>Normal</span>
            </div>
            <input type="range" class="w-full accent-pink-500" value="50" min="0" max="100" />
          </div>
        </div>
      </div>

      <div
        class="rounded-xl p-6 mb-6 backdrop-blur-sm"
        style="background-color: var(--bg-secondary); border: 1px solid var(--border-color)"
      >
        <h2 class="text-lg font-semibold mb-4" style="color: var(--text-primary)">Account</h2>
        <button
          @click="handleLogout"
          class="w-full py-3 rounded-lg bg-red-500/20 text-red-400 font-medium hover:bg-red-500/30 transition-colors text-sm"
        >
          ❌ Logout
        </button>
      </div>

      <p class="text-center text-xs" style="color: var(--text-secondary)">
        v1.0.0 Cyberpunk Edition
      </p>
    </div>
  </div>
</template>
