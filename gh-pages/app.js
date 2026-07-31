'use strict'
const pickButton = document.getElementById('pick')
const recentButton = document.getElementById('recent')
const folderInput = document.getElementById('folder')
const startScreen = document.getElementById('start')
const gameEl = document.getElementById('game')
const logEl = document.getElementById('log')
const canvas = document.getElementById('canvas')
const errorDialog = document.getElementById('error-dialog')
const errorDialogPick = document.getElementById('error-dialog-pick')
const errorDialogBack = document.getElementById('error-dialog-back')
canvas.addEventListener('contextmenu', (e) => e.preventDefault())

let startingUp = false
let engineLoadFailed = false
let engine = null
let saveSyncTimer = null
let saveSyncInProgress = false
let saveSnapshot = new Map()

const SAVE_SYNC_INTERVAL_MS = 3000

const DB_NAME = 'openshadowflare'
const DB_VERSION = 1
const FILES_STORE = 'files'

let dbPromise = null

function openDb() {
  if (dbPromise) return dbPromise
  dbPromise = new Promise((resolve, reject) => {
    const request = indexedDB.open(DB_NAME, DB_VERSION)
    request.onupgradeneeded = () => {
      const db = request.result
      if (!db.objectStoreNames.contains(FILES_STORE)) {
        db.createObjectStore(FILES_STORE, { keyPath: 'path' })
      }
    }
    request.onsuccess = () => resolve(request.result)
    request.onerror = () => reject(request.error)
  })
  return dbPromise
}

async function hasRecentFiles() {
  try {
    const db = await openDb()
    return await new Promise((resolve, reject) => {
      const tx = db.transaction(FILES_STORE, 'readonly')
      const request = tx.objectStore(FILES_STORE).count()
      request.onsuccess = () => resolve(request.result > 0)
      request.onerror = () => reject(request.error)
    })
  } catch (error) {
    console.warn('[storage] could not inspect recent files:', error)
    return false
  }
}

async function loadRecentFiles() {
  const db = await openDb()
  const records = await new Promise((resolve, reject) => {
    const tx = db.transaction(FILES_STORE, 'readonly')
    const request = tx.objectStore(FILES_STORE).getAll()
    request.onsuccess = () => resolve(request.result || [])
    request.onerror = () => reject(request.error)
  })
  return records
    .filter((record) => record && record.path && record.bytes)
    .map((record) => ({
      path: record.path,
      bytes:
        record.bytes instanceof ArrayBuffer
          ? new Uint8Array(record.bytes)
          : new Uint8Array(
              record.bytes.buffer,
              record.bytes.byteOffset,
              record.bytes.byteLength
            )
    }))
}

async function storeRecentFiles(entries) {
  const db = await openDb()
  await new Promise((resolve, reject) => {
    const tx = db.transaction(FILES_STORE, 'readwrite')
    const store = tx.objectStore(FILES_STORE)
    store.clear()
    for (const entry of entries) {
      store.put({ path: entry.path, bytes: entry.bytes })
    }
    tx.oncomplete = () => resolve()
    tx.onerror = () => reject(tx.error)
  })
}

function getSaveDir(FS) {
  try {
    const save = FS.readdir('/game').find(
      (name) => name !== '.' && name !== '..' && name.toLowerCase() === 'save'
    )
    return save ? '/game/' + save : '/game/Save'
  } catch (error) {
    return '/game/Save'
  }
}

const SAVE_SYNC_TARGETS = [
  {
    matcher: /\.(ssv|bmp)$/i,
    resolveDir: (FS) => getSaveDir(FS),
    keyPrefix: (dir) => dir.slice('/game/'.length) + '/'
  },
  {
    matcher: /^Sflare\.cfg$/i,
    resolveDir: () => '/game',
    keyPrefix: () => ''
  }
]

function startSaveSync(FS) {
  return setInterval(() => {
    syncSaveFiles(FS)
  }, SAVE_SYNC_INTERVAL_MS)
}

async function syncSaveFiles(FS) {
  if (saveSyncInProgress) return
  const seen = new Set()
  const updates = []

  for (const target of SAVE_SYNC_TARGETS) {
    const dir = target.resolveDir(FS)
    let names
    try {
      names = FS.readdir(dir)
    } catch (error) {
      continue
    }
    const keyPrefix = target.keyPrefix(dir)
    for (const name of names) {
      if (name === '.' || name === '..' || !target.matcher.test(name)) continue
      const key = keyPrefix + name
      seen.add(key)
      try {
        const stat = FS.stat(dir + '/' + name)
        const mtime = +stat.mtime
        const cached = saveSnapshot.get(key)
        if (cached && cached.mtime === mtime && cached.size === stat.size) {
          continue
        }
        const bytes = new Uint8Array(FS.readFile(dir + '/' + name))
        updates.push({ key, bytes })
        saveSnapshot.set(key, { mtime, size: stat.size })
      } catch (error) {
        console.warn('[savesync] could not read ' + dir + '/' + name, error)
      }
    }
  }

  for (const key of saveSnapshot.keys()) {
    if (!seen.has(key)) {
      updates.push({ key, remove: true })
    }
  }
  for (const key of [...saveSnapshot.keys()]) {
    if (!seen.has(key)) saveSnapshot.delete(key)
  }

  if (updates.length === 0) return
  saveSyncInProgress = true
  try {
    await persistSaveChanges(updates)
  } catch (error) {
    console.warn('[savesync] could not persist save changes:', error)
  } finally {
    saveSyncInProgress = false
  }
}

async function persistSaveChanges(updates) {
  const db = await openDb()
  await new Promise((resolve, reject) => {
    const tx = db.transaction(FILES_STORE, 'readwrite')
    const store = tx.objectStore(FILES_STORE)
    for (const update of updates) {
      if (update.remove) {
        store.delete(update.key)
      } else {
        store.put({ path: update.key, bytes: update.bytes })
      }
    }
    tx.oncomplete = () => resolve()
    tx.onerror = () => reject(tx.error)
  })
}

function appendLog(text) {
  logEl.textContent += text + '\n'
  logEl.scrollTop = logEl.scrollHeight
}

function stopEngine() {
  clearInterval(saveSyncTimer)
  saveSyncTimer = null
  if (engine && typeof engine.pauseMainLoop === 'function') {
    engine.pauseMainLoop()
  }
  engine = null
}

function showErrorDialog() {
  stopEngine()
  errorDialog.hidden = false
}

function resetToStart() {
  stopEngine()
  errorDialog.hidden = true
  logEl.textContent = ''
  startScreen.hidden = false
  gameEl.hidden = true
}

function enterLoading(message) {
  stopEngine()
  errorDialog.hidden = true
  startScreen.hidden = true
  gameEl.hidden = false
  appendLog(message)
}

pickButton.addEventListener('click', () => folderInput.click())
errorDialogPick.addEventListener('click', () => {
  resetToStart()
  folderInput.click()
})
errorDialogBack.addEventListener('click', resetToStart)
document.addEventListener('keydown', (event) => {
  if (event.key === 'Escape' && !errorDialog.hidden) {
    resetToStart()
  }
})

window.addEventListener('unhandledrejection', (event) => {
  if (
    event.reason instanceof DOMException &&
    event.reason.name === 'InvalidStateError'
  ) {
    event.preventDefault()
  }
})

folderInput.addEventListener('change', async (event) => {
  const files = Array.from(event.target.files || [])
  if (files.length === 0) return
  enterLoading('Reading game files…')
  const entries = await readFolderFiles(files)
  startGame(entries)
})

recentButton.addEventListener('click', async () => {
  enterLoading('Loading saved game files…')
  let records
  try {
    records = await loadRecentFiles()
  } catch (error) {
    console.warn('[storage] could not load recent files:', error)
    appendLog('[storage] could not load recent files.')
    resetToStart()
    return
  }
  if (records.length === 0) {
    recentButton.hidden = true
    resetToStart()
    return
  }
  appendLog(`Loaded ${records.length} saved game files.`)
  startGame(records, { fromRecent: true })
})

hasRecentFiles().then((exists) => {
  recentButton.hidden = !exists
})

const GAME_FOLDERS = [
  'Character',
  'Map',
  'Player',
  'Save',
  'Scenario',
  'System'
]
const GAME_ROOT_FILES = ['SFlare.Cfg']

async function readFolderFiles(files) {
  const entries = []
  for (const file of files) {
    const relative = file.webkitRelativePath || file.name
    const slash = relative.indexOf('/')
    const inner = slash >= 0 ? relative.slice(slash + 1) : relative
    const top = inner.slice(0, inner.indexOf('/'))
    const fileName = inner.slice(inner.lastIndexOf('/') + 1)
    const isGameFolder = GAME_FOLDERS.some(
      (name) => name.toLowerCase() === top.toLowerCase()
    )
    const isGameRootFile = GAME_ROOT_FILES.some(
      (rootName) => rootName.toLowerCase() === fileName.toLowerCase()
    )
    if (!isGameFolder && !isGameRootFile) continue
    entries.push({
      path: inner,
      bytes: new Uint8Array(await file.arrayBuffer())
    })
  }
  return entries
}

async function startGame(entries, options = {}) {
  folderInput.value = ''
  stopEngine()
  errorDialog.hidden = true
  startingUp = true
  engineLoadFailed = false
  startScreen.hidden = true
  gameEl.hidden = false
  appendLog('Loading engine…')

  let ModuleInstance
  try {
    ModuleInstance = await ShadowFlareModule({
      canvas,
      noInitialRun: true,
      print: appendLog,
      printErr: (text) => {
        appendLog('[stderr] ' + text)
        if (startingUp && /Could not load .\/System/i.test(text)) {
          engineLoadFailed = true
        }
      }
    })
  } catch (error) {
    startingUp = false
    appendLog('Could not load the engine.')
    appendLog(String(error))
    return
  }
  engine = ModuleInstance

  const FS = ModuleInstance.FS
  appendLog(`Mounting ${entries.length} game files…`)

  for (const entry of entries) {
    const destination = '/game/' + entry.path
    const directory = destination.slice(0, destination.lastIndexOf('/'))
    try {
      FS.mkdirTree(directory)
      FS.writeFile(destination, entry.bytes)
    } catch (error) {
      appendLog('[mount] ' + destination + ': ' + error)
    }
  }

  saveSnapshot = new Map()
  saveSyncTimer = startSaveSync(FS)

  appendLog('Starting game…')
  try {
    FS.chdir('/game')
  } catch (error) {
    appendLog('[chdir] ' + error)
  }

  canvas.focus()
  try {
    ModuleInstance.callMain([])
  } catch (error) {
    appendLog('The game exited.')
    appendLog(String(error))
  }
  startingUp = false
  if (engineLoadFailed) {
    showErrorDialog()
    return
  }
  if (!options.fromRecent) {
    try {
      await storeRecentFiles(entries)
      recentButton.hidden = false
    } catch (error) {
      console.warn('[storage] could not store recent files:', error)
    }
  }
}
