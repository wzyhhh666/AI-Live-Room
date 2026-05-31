import { ref, watch } from 'vue'

export type Theme = 'dark' | 'light' | 'warm'

const THEME_STORAGE_KEY = 'ai-live-theme'
const currentTheme = ref<Theme>('dark')

const cssVars: Record<Theme, Record<string, string>> = {
  dark: {
    '--bg-primary': '#0F0F23',
    '--bg-secondary': '#1a1a2e',
    '--text-primary': '#FFFFFF',
    '--text-secondary': '#9CA3AF',
    '--border-color': '#374151',
    '--accent-pink': '#FF006E',
    '--accent-purple': '#8338EC',
    '--accent-cyan': '#00D4FF',
  },
  light: {
    '--bg-primary': '#F8FAFC',
    '--bg-secondary': '#FFFFFF',
    '--text-primary': '#1E293B',
    '--text-secondary': '#64748B',
    '--border-color': '#E2E8F0',
    '--accent-pink': '#DB2777',
    '--accent-purple': '#7C3AED',
    '--accent-cyan': '#0891B2',
  },
  warm: {
    '--bg-primary': '#1A3320',
    '--bg-secondary': '#2A4A30',
    '--text-primary': '#E8F5E9',
    '--text-secondary': '#A5D6A7',
    '--border-color': '#388E3C',
    '--accent-pink': '#E91E63',
    '--accent-purple': '#9C27B0',
    '--accent-cyan': '#00BCD4',
  },
}

function applyTheme(theme: Theme) {
  const vars = cssVars[theme]
  const root = document.documentElement
  for (const [key, value] of Object.entries(vars)) {
    root.style.setProperty(key, value)
  }
  localStorage.setItem(THEME_STORAGE_KEY, theme)
  currentTheme.value = theme
  document.documentElement.setAttribute('data-theme', theme)
}

function setTheme(theme: Theme) {
  applyTheme(theme)
}

function initFromStorage() {
  const saved = localStorage.getItem(THEME_STORAGE_KEY) as Theme | null
  applyTheme(saved || 'dark')
}

const isDark = () => currentTheme.value === 'dark'
const isLight = () => currentTheme.value === 'light'
const isWarm = () => currentTheme.value === 'warm'

export function useTheme() {
  return {
    theme: currentTheme,
    setTheme,
    isDark,
    isLight,
    isWarm,
    initFromStorage,
  }
}
