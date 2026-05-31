class SensitiveFilter {
  constructor() {
    this.trie = {}
    this.wordList = []
  }

  init() {
    const data = require('../../config/sensitive_words.json')
    const words = data.words || data
    this.wordList = words
    for (const word of words) {
      this.addWord(word)
    }
    console.log(`🔍 Sensitive filter initialized with ${words.length} words`)
  }

  addWord(word) {
    let node = this.trie
    for (const char of word) {
      if (!node[char]) node[char] = {}
      node = node[char]
    }
    node.isEnd = true
  }

  filter(text) {
    let result = ''
    let i = 0
    const matchedWords = []

    while (i < text.length) {
      let node = this.trie
      let j = i
      let matchLen = 0

      while (j < text.length && node[text[j]]) {
        node = node[text[j]]
        j++
        if (node.isEnd) {
          matchLen = j - i
        }
      }

      if (matchLen > 0) {
        matchedWords.push(text.substring(i, i + matchLen))
        result += '*'.repeat(matchLen)
        i += matchLen
      } else {
        result += text[i]
        i++
      }
    }

    return { filtered: result, matched: matchedWords }
  }

  async filterWithLog(roomId, userId, content, dbPool) {
    const result = this.filter(content)

    if (result.matched.length > 0) {
      try {
        const matchedStr = result.matched.join(',')
        const action = result.filtered !== content ? 'mask' : 'block'

        await dbPool.execute(
          `INSERT INTO sensitive_log (room_id, user_id, original_content, filtered_content, matched_words, action)
           VALUES (?, ?, ?, ?, ?, ?)`,
          [roomId, userId || null, content, result.filtered, matchedStr, action]
        )
      } catch (err) {
        console.error('Failed to write sensitive_log:', err.message)
      }
    }

    return result
  }
}

module.exports = new SensitiveFilter()
