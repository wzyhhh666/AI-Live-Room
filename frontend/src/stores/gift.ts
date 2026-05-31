import { defineStore } from 'pinia'
import { ref } from 'vue'
import apiService from '@/services/api'

export interface GiftItem {
  id: number
  giftName: string
  giftIcon: string
  price: number
  effectType: 'normal' | 'explosion' | 'rain' | 'rocket'
  sortOrder: number
}

export interface GiftRankEntry {
  senderId: number
  senderName: string
  totalAmount: number
}

export interface GiftEvent {
  id: number
  giftName: string
  giftIcon: string
  count: number
  totalPrice: number
  senderName: string
  effectType: string
  timestamp: number
}

export const useGiftStore = defineStore('gift', () => {
  const giftList = ref<GiftItem[]>([])
  const selectedGift = ref<GiftItem | null>(null)
  const sendCount = ref(1)
  const giftRank = ref<GiftRankEntry[]>([])
  const lastGift = ref<GiftEvent | null>(null)

  const totalPrice = ref(0)

  async function fetchGiftList() {
    const result = await apiService.getGiftList()
    if (result.success && result.data) {
      giftList.value = result.data
    }
  }

  function selectGift(gift: GiftItem) {
    selectedGift.value = gift
    updateTotalPrice()
  }

  function setCount(n: number) {
    sendCount.value = n
    updateTotalPrice()
  }

  function updateTotalPrice() {
    if (selectedGift.value) {
      totalPrice.value = selectedGift.value.price * sendCount.value
    }
  }

  function sendGift(roomId: string): { success: false, error: string } | { success: true } {
    console.log('[giftStore.sendGift] called with roomId:', roomId)
    console.log('[giftStore.sendGift] selectedGift:', selectedGift.value?.giftName)
    
    if (!selectedGift.value) {
      console.log('[giftStore.sendGift] FAIL: no gift selected')
      return { success: false, error: '请先选择礼物' }
    }

    const socket = apiService.getSocket()
    console.log('[giftStore.sendGift] socket:', socket?.id, 'connected:', socket?.connected)
    
    if (!socket) {
      console.log('[giftStore.sendGift] FAIL: socket is null')
      return { success: false, error: '正在连接直播间，请稍后再试' }
    }
    if (!socket.connected) {
      console.log('[giftStore.sendGift] FAIL: socket not connected')
      return { success: false, error: '连接已断开，请刷新页面' }
    }

    const payload = {
      roomId,
      giftId: selectedGift.value.id,
      giftName: selectedGift.value.giftName,
      count: sendCount.value,
      price: selectedGift.value.price * sendCount.value,
      effectType: selectedGift.value.effectType
    }
    console.log('[giftStore.sendGift] emitting send-gift:', JSON.stringify(payload))
    socket.emit('send-gift', payload)
    console.log('[giftStore.sendGift] emit done')
    return { success: true }
  }

  function updateRank(entries: GiftRankEntry[]) {
    // #region debug-point D:store-rank
    fetch("http://127.0.0.1:7778/event",{method:"POST",body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"D",location:"gift.ts:81",msg:"[DEBUG] giftStore.updateRank called",data:{oldLen:giftRank.value.length,newLen:entries.length}})}).catch(()=>{});
    // #endregion
    giftRank.value = entries
    // #region debug-point D:store-done
    fetch("http://127.0.0.1:7778/event",{method:"POST",body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"D",location:"gift.ts:84",msg:"[DEBUG] giftStore.updateRank done",data:{length:giftRank.value.length}})}).catch(()=>{});
    // #endregion
  }

  function triggerEffect(event: GiftEvent) {
    lastGift.value = { ...event, timestamp: Date.now() }
  }

  return {
    giftList,
    selectedGift,
    sendCount,
    giftRank,
    lastGift,
    totalPrice,
    fetchGiftList,
    selectGift,
    setCount,
    sendGift,
    updateRank,
    triggerEffect
  }
})
