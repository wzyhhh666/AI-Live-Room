export async function startWhipStream(streamKey: string): Promise<{ pc: RTCPeerConnection; mediaStream: MediaStream }> {
  const mediaStream = await navigator.mediaDevices.getUserMedia({
    video: {
      width: { ideal: 1280 },
      height: { ideal: 720 },
      frameRate: { ideal: 25 }
    },
    audio: {
      echoCancellation: true,
      noiseSuppression: true,
      sampleRate: 48000
    }
  })

  const pc = new RTCPeerConnection({
    iceServers: [],
    iceTransportPolicy: 'all',
    bundlePolicy: 'max-bundle',
    rtcpMuxPolicy: 'require'
  })

  let iceConnected = false
  let iceFailed = false

  pc.oniceconnectionstatechange = () => {
    console.log('[WHIP] ICE state:', pc.iceConnectionState)
    if (pc.iceConnectionState === 'connected' || pc.iceConnectionState === 'completed') {
      iceConnected = true
    }
    if (pc.iceConnectionState === 'failed' || pc.iceConnectionState === 'disconnected') {
      iceFailed = true
    }
  }

  pc.onconnectionstatechange = () => {
    console.log('[WHIP] Connection state:', pc.connectionState)
  }

  for (const track of mediaStream.getTracks()) {
    pc.addTrack(track, mediaStream)
  }

  for (const transceiver of pc.getTransceivers()) {
    if (transceiver.sender?.track?.kind === 'video') {
      try {
        const codecs = RTCRtpSender.getCapabilities('video')?.codecs || []
        const h264Codecs = codecs.filter(c =>
          c.mimeType === 'video/H264'
        ).sort((a, b) => {
          const aLevel = parseInt(a.sdpFmtpLine?.match(/profile-level-id=([0-9a-fA-F]+)/)?.[1] || '0', 16)
          const bLevel = parseInt(b.sdpFmtpLine?.match(/profile-level-id=([0-9a-fA-F]+)/)?.[1] || '0', 16)
          return bLevel - aLevel
        })
        if (h264Codecs.length > 0) {
          transceiver.setCodecPreferences([...h264Codecs, ...codecs.filter(c => c.mimeType !== 'video/H264')])
          console.log('[WHIP] Set H.264 codec preference, available profiles:', h264Codecs.length)
        } else {
          console.warn('[WHIP] H.264 not available in browser, using default codecs')
        }
      } catch (e) {
        console.warn('[WHIP] setCodecPreferences not supported, falling back to default codecs')
      }
    }
  }

  const offer = await pc.createOffer({
    offerToReceiveAudio: false,
    offerToReceiveVideo: false
  })
  await pc.setLocalDescription(offer)

  await waitForIceGathering(pc)

  const srsRtcUrl = import.meta.env.VITE_SRS_RTC_URL || 'https://localhost:1985'
  const whipUrl = `${srsRtcUrl}/rtc/v1/whip/?app=live&stream=${streamKey}`

  const response = await fetch(whipUrl, {
    method: 'POST',
    body: pc.localDescription?.sdp,
    headers: { 'Content-Type': 'application/sdp' }
  })

  if (!response.ok) {
    const errText = await response.text()
    mediaStream.getTracks().forEach(t => t.stop())
    pc.close()
    throw new Error(`WHIP request failed: ${response.status} ${errText}`)
  }

  const answerSdp = await response.text()
  await pc.setRemoteDescription({ type: 'answer', sdp: answerSdp })

  try {
    await new Promise<void>((resolve, reject) => {
      const timeout = setTimeout(() => {
        if (iceConnected) {
          resolve()
        } else {
          reject(new Error('ICE connection timeout'))
        }
      }, 15000)

      const checkInterval = setInterval(() => {
        if (iceConnected) {
          clearTimeout(timeout)
          clearInterval(checkInterval)
          resolve()
        }
        if (iceFailed) {
          clearTimeout(timeout)
          clearInterval(checkInterval)
          reject(new Error('ICE connection failed'))
        }
      }, 500)
    })

    console.log('[WHIP] ICE connected, stream is live')
  } catch (iceErr: any) {
    stopWhipStream(pc, mediaStream)
    throw new Error(`WebRTC connection failed: ${iceErr.message}`)
  }

  return { pc, mediaStream }
}

function waitForIceGathering(pc: RTCPeerConnection): Promise<void> {
  return new Promise((resolve) => {
    if (pc.iceGatheringState === 'complete') {
      resolve()
      return
    }
    const handler = () => {
      if (pc.iceGatheringState === 'complete') {
        pc.removeEventListener('icegatheringstatechange', handler)
        resolve()
      }
    }
    pc.addEventListener('icegatheringstatechange', handler)
    const timeout = setTimeout(() => {
      pc.removeEventListener('icegatheringstatechange', handler)
      resolve()
    }, 3000)
  })
}

export function stopWhipStream(pc: RTCPeerConnection | null, mediaStream: MediaStream | null) {
  if (mediaStream) {
    mediaStream.getTracks().forEach(t => t.stop())
  }
  if (pc) {
    pc.close()
  }
}
