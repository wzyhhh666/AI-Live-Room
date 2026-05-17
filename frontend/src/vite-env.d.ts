/// <reference types="vite/client" />

declare module '*.vue' {
  import type { DefineComponent } from 'vue'
  const component: DefineComponent<{}, {}, any>
  export default component
}

interface Window {
  electronAPI: {
    getVersion: () => Promise<string>
    windowControl: {
      minimize: () => void
      maximize: () => void
      close: () => void
    }
    platform: string
  }
}
