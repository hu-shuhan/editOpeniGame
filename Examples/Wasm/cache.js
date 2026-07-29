const BUILD_ID = '@IGAME_WASM_BUILD_ID@';
const MEMORY_PROFILE = '@IGAME_WASM_MEMORY_PROFILE@';
const OWNER_LEASE_TIMEOUT_MS = 120000;
const MAX_ARTIFACT_COUNT = 2;
const MAX_ARTIFACT_BYTES = 512 * 1024 * 1024;

const entries = new Map();
const ports = new Map();

const post = (port, message) => {
    try {
        port.postMessage(message);
        return true;
    } catch (_error) {
        return false;
    }
};

const artifactByteLength = (artifact) => {
    if (!artifact || artifact.version !== 1) {
        return 0;
    }
    const buffers = [artifact.positions, artifact.triangles, artifact.edgeMasks];
    if (buffers.some((buffer) => !buffer || typeof buffer.byteLength !== 'number')) {
        return 0;
    }
    return buffers.reduce((sum, buffer) => sum + buffer.byteLength, 0);
};

const pruneArtifacts = (protectedKey) => {
    const artifacts = Array.from(entries.entries())
        .filter(([contentKey, entry]) => contentKey !== protectedKey && entry.artifact)
        .sort((lhs, rhs) => lhs[1].lastAccess - rhs[1].lastAccess);
    let artifactCount = artifacts.length + (entries.get(protectedKey)?.artifact ? 1 : 0);
    let residentBytes = Array.from(entries.values())
        .reduce((sum, entry) => sum + Number(entry.artifactBytes || 0), 0);
    for (const [contentKey, entry] of artifacts) {
        if (artifactCount <= MAX_ARTIFACT_COUNT && residentBytes <= MAX_ARTIFACT_BYTES) {
            break;
        }
        entries.delete(contentKey);
        artifactCount -= 1;
        residentBytes -= Number(entry.artifactBytes || 0);
    }
};

const grantNextOwner = (contentKey, entry, previousResult) => {
    while (entry.queue.length > 0) {
        const candidate = entry.queue.shift();
        if (!ports.has(candidate.pageId)) {
            continue;
        }
        entry.ownerPageId = candidate.pageId;
        entry.ownerPort = candidate.port;
        entry.lastHeartbeat = Date.now();
        if (post(candidate.port, {
            type: 'owner',
            requestId: candidate.requestId,
            contentKey,
            ownerPageId: candidate.pageId,
            previousResult,
        })) {
            return;
        }
        ports.delete(candidate.pageId);
    }
    entries.delete(contentKey);
};

const releaseOwner = (contentKey, entry, success, detail) => {
    grantNextOwner(contentKey, entry, {
        success: Boolean(success),
        detail: String(detail || ''),
    });
};

const publishArtifact = (contentKey, entry, artifact) => {
    const residentBytes = artifactByteLength(artifact);
    if (residentBytes === 0 || residentBytes > MAX_ARTIFACT_BYTES) {
        return false;
    }
    entry.ownerPageId = '';
    entry.ownerPort = null;
    entry.lastHeartbeat = 0;
    entry.artifact = artifact;
    entry.artifactBytes = residentBytes;
    entry.lastAccess = Date.now();
    for (const candidate of entry.queue) {
        post(candidate.port, {
            type: 'artifact',
            requestId: candidate.requestId,
            contentKey,
            artifact,
            residentBytes,
        });
    }
    entry.queue = [];
    pruneArtifacts(contentKey);
    return true;
};

const removePage = (pageId) => {
    ports.delete(pageId);
    for (const [contentKey, entry] of entries) {
        entry.queue = entry.queue.filter((candidate) => candidate.pageId !== pageId);
        if (entry.ownerPageId === pageId) {
            releaseOwner(contentKey, entry, false, 'owner page disconnected');
        }
    }
};

const handleMessage = (port, message) => {
    const type = String(message && message.type || '');
    const pageId = String(message && message.pageId || '');
    if (type === 'register') {
        if (!pageId) {
            return;
        }
        ports.set(pageId, port);
        post(port, {
            type: 'registered',
            requestId: message.requestId,
            pageId,
            buildId: BUILD_ID,
            memoryProfile: MEMORY_PROFILE,
        });
        return;
    }
    if (!pageId || ports.get(pageId) !== port) {
        return;
    }
    if (type === 'disconnect') {
        removePage(pageId);
        return;
    }
    const contentKey = String(message && message.contentKey || '');
    if (type === 'acquire') {
        if (!contentKey) {
            post(port, { type: 'error', requestId: message.requestId, detail: 'empty content key' });
            return;
        }
        const entry = entries.get(contentKey);
        if (!entry) {
            entries.set(contentKey, {
                ownerPageId: pageId,
                ownerPort: port,
                lastHeartbeat: Date.now(),
                queue: [],
                artifact: null,
                artifactBytes: 0,
                lastAccess: Date.now(),
            });
            post(port, {
                type: 'owner',
                requestId: message.requestId,
                contentKey,
                ownerPageId: pageId,
            });
            return;
        }
        if (entry.artifact) {
            entry.lastAccess = Date.now();
            post(port, {
                type: 'artifact',
                requestId: message.requestId,
                contentKey,
                artifact: entry.artifact,
                residentBytes: entry.artifactBytes,
            });
            return;
        }
        if (entry.ownerPageId === pageId) {
            entry.lastHeartbeat = Date.now();
            post(port, {
                type: 'owner',
                requestId: message.requestId,
                contentKey,
                ownerPageId: pageId,
            });
            return;
        }
        entry.queue.push({ pageId, port, requestId: message.requestId });
        post(port, {
            type: 'queued',
            requestId: message.requestId,
            contentKey,
            ownerPageId: entry.ownerPageId,
            queuePosition: entry.queue.length,
        });
        return;
    }
    const entry = entries.get(contentKey);
    if (!entry || entry.ownerPageId !== pageId) {
        return;
    }
    if (type === 'heartbeat') {
        entry.lastHeartbeat = Date.now();
        return;
    }
    if (type === 'complete') {
        if (message.success && message.artifact) {
            if (publishArtifact(contentKey, entry, message.artifact)) {
                post(port, {
                    type: 'artifact-published',
                    contentKey,
                    residentBytes: artifactByteLength(message.artifact),
                });
                return;
            }
            post(port, {
                type: 'artifact-rejected',
                contentKey,
                detail: 'invalid or oversized shared surface artifact',
            });
        }
        releaseOwner(contentKey, entry, message.success, message.detail);
        return;
    }
};

setInterval(() => {
    const now = Date.now();
    for (const [contentKey, entry] of entries) {
        if (entry.ownerPageId && now - entry.lastHeartbeat > OWNER_LEASE_TIMEOUT_MS) {
            releaseOwner(contentKey, entry, false, 'owner heartbeat expired');
        }
    }
}, 15000);

self.onconnect = (event) => {
    const port = event.ports[0];
    port.onmessage = (messageEvent) => handleMessage(port, messageEvent.data);
    port.onmessageerror = () => {
        for (const [pageId, registeredPort] of ports) {
            if (registeredPort === port) {
                removePage(pageId);
                break;
            }
        }
    };
    port.start();
};
