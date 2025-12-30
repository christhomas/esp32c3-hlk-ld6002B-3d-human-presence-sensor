// ========== CONFIGURATION ==========
const MAX_TARGET_HISTORY = 20;  // Maximum number of targets to track
const MAX_TRAIL_LENGTH = 100;   // Maximum history steps per target
const TARGET_MATCH_DISTANCE = 0.5;  // Distance threshold for matching targets (meters)

// ========== GLOBAL VARIABLES ==========
let scene, camera, renderer, grid, zones = [];
let targets = [], targetCards = [];
let stats = { frames: 0, lastTime: Date.now(), fps: 0 };
let gridVisible = true, zonesVisible = true;
let deviceArrow, sceneRoot;
let targetHistory = {}, trailLines = [], targetLines = [], maxTrailLength = 50;
let targetHistoryOrder = [];
let nextTargetId = 1;  // Incremental ID for new targets

// Zone visualization
let detectionZoneMeshes = [];
let interferenceZoneMeshes = [];
let zoneLabels = [];

const targetColors = [
    0x4fc3f7, 0xff9800, 0x4caf50, 0xe91e63, 0x9c27b0,
    0x00bcd4, 0xffeb3b, 0xff5722, 0x795548, 0x607d8b
];

// ========== HELPER FUNCTIONS ==========
function getTargetColor(id) {
    return targetColors[parseInt(id) % targetColors.length];
}

function removeOldestTarget() {
    if (targetHistoryOrder.length > 0) {
        const oldestId = targetHistoryOrder.shift();
        if (targetHistory[oldestId] && targetHistory[oldestId].sphere) {
            sceneRoot.remove(targetHistory[oldestId].sphere);
        }
        delete targetHistory[oldestId];
        console.log('Removed oldest target:', oldestId);
    }
}

// ========== SCENE INITIALIZATION ==========
function init() {
    // Create scene
    scene = new THREE.Scene();
    scene.background = new THREE.Color(0x0a0e27);

    // Create scene root with rotation
    sceneRoot = new THREE.Group();
    const rotQuat = new THREE.Quaternion();
    rotQuat.setFromAxisAngle(new THREE.Vector3(1, 0, 0), -Math.PI / 2);
    sceneRoot.quaternion.copy(rotQuat);
    scene.add(sceneRoot);

    // Setup camera
    camera = new THREE.PerspectiveCamera(
        75,
        window.innerWidth / window.innerHeight,
        0.1,
        100
    );
    camera.position.set(0, 3, -3);
    camera.lookAt(0, 0, 0);

    // Setup renderer
    renderer = new THREE.WebGLRenderer({
        canvas: document.getElementById('canvas'),
        antialias: true
    });
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.setPixelRatio(window.devicePixelRatio);

    // Add lighting
    scene.add(new THREE.AmbientLight(0xffffff, 0.6));
    const dl = new THREE.DirectionalLight(0xffffff, 0.8);
    dl.position.set(5, 5, 5);
    scene.add(dl);

    // Add grid and axes
    grid = new THREE.GridHelper(10, 20, 0x4fc3f7, 0x2c3e50);
    scene.add(grid);
    scene.add(new THREE.AxesHelper(2));

    // Create device direction arrow
    deviceArrow = new THREE.Group();
    const arrowMat = new THREE.MeshBasicMaterial({ 
        color: 0xff4444, 
        side: THREE.DoubleSide 
    });

    // Arrow shaft
    const shaftGeo = new THREE.BoxGeometry(0.06, 1.2, 0.01);
    const shaft = new THREE.Mesh(shaftGeo, arrowMat);
    shaft.position.set(0, 0.6, 0);
    deviceArrow.add(shaft);

    // Arrow head
    const headShape = new THREE.Shape();
    headShape.moveTo(0, 1.3);
    headShape.lineTo(-0.15, 1.15);
    headShape.lineTo(0.15, 1.15);
    headShape.lineTo(0, 1.3);
    const headGeo = new THREE.ShapeGeometry(headShape);
    const head = new THREE.Mesh(headGeo, arrowMat);
    head.position.set(0, 0, 0.005);
    deviceArrow.add(head);

    // "FRONT" label
    const canvas = document.createElement('canvas');
    canvas.width = 256;
    canvas.height = 64;
    const ctx = canvas.getContext('2d');
    ctx.fillStyle = '#ff4444';
    ctx.font = 'bold 48px Arial';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('FRONT', 128, 32);
    const texture = new THREE.CanvasTexture(canvas);
    const spriteMat = new THREE.SpriteMaterial({ map: texture });
    const sprite = new THREE.Sprite(spriteMat);
    sprite.position.set(0, 1.5, 0);
    sprite.scale.set(0.5, 0.125, 1);
    deviceArrow.add(sprite);
    sceneRoot.add(deviceArrow);

    // Create placeholder zone boxes (will be updated with real data)
    for (let i = 0; i < 4; i++) {
        const z = new THREE.Mesh(
            new THREE.BoxGeometry(2, 0.05, 2),
            new THREE.MeshBasicMaterial({
                color: 0x4fc3f7,
                transparent: true,
                opacity: 0.3,
                wireframe: true
            })
        );
        z.position.set(0, 0.5, 0);
        z.visible = false;  // Hide until we get real zone data
        sceneRoot.add(z);
        zones.push(z);
    }

    // Setup event listeners
    window.addEventListener('resize', () => {
        camera.aspect = window.innerWidth / window.innerHeight;
        camera.updateProjectionMatrix();
        renderer.setSize(window.innerWidth, window.innerHeight);
    });

    // Mouse controls
    let drag = false, prev = { x: 0, y: 0 };
    renderer.domElement.addEventListener('mousedown', e => {
        drag = true;
        prev = { x: e.clientX, y: e.clientY };
    });

    renderer.domElement.addEventListener('mousemove', e => {
        if (drag) {
            const dx = e.clientX - prev.x;
            const dy = e.clientY - prev.y;
            camera.position.applyAxisAngle(new THREE.Vector3(0, 1, 0), -dx * 0.005);
            const r = new THREE.Vector3();
            r.crossVectors(camera.up, camera.position).normalize();
            camera.position.applyAxisAngle(r, -dy * 0.005);
            camera.lookAt(0, 0, 0);
            prev = { x: e.clientX, y: e.clientY };
        }
    });

    renderer.domElement.addEventListener('mouseup', () => {
        drag = false;
    });

    renderer.domElement.addEventListener('wheel', e => {
        e.preventDefault();
        const dir = camera.position.clone().normalize();
        camera.position.addScaledVector(dir, e.deltaY > 0 ? 0.1 : -0.1);
    });

    connectSSE();
    animate();
}

// ========== SSE CONNECTION ==========
function connectSSE() {
    const es = new EventSource('/events');
    
    es.onopen = () => {
        document.getElementById('status').textContent = 'Connected';
        document.getElementById('status').className = 'value connected';
        console.log('SSE connected');
    };

    es.onerror = () => {
        document.getElementById('status').textContent = 'Reconnecting...';
        document.getElementById('status').className = 'value disconnected';
        setTimeout(() => connectSSE(), 2000);
    };

    es.onmessage = e => {
        try {
            const msg = JSON.parse(e.data);
            stats.frames++;
            document.getElementById('frame-count').textContent = stats.frames;

            if (msg.type === 'target') {
                updateTargets(msg.data);
                document.getElementById('target-count').textContent = msg.data.length;
            } else if (msg.type === 'presence') {
                updatePresence(msg.data);
            } else if (msg.type === 'detection_zones') {
                updateDetectionZones(msg.data);
            } else if (msg.type === 'interference_zones') {
                updateInterferenceZones(msg.data);
            } else if (msg.type === 'config') {
                updateConfigUI(msg.data);
            }
        } catch (err) {
            console.error('Parse error:', err);
        }
    };
}

// ========== TARGET UPDATES ==========
function updateTargets(data) {
    // Clear cards and rebuild targets array
    targetCards.forEach(c => c.remove());
    targetCards = [];
    targets = [];
    
    // Clear connecting lines
    targetLines.forEach(line => sceneRoot.remove(line));
    targetLines = [];

    // Calculate distance between two 3D points
    function distance3D(p1, p2) {
        const dx = p1.x - p2.x;
        const dy = p1.y - p2.y;
        const dz = p1.z - p2.z;
        return Math.sqrt(dx * dx + dy * dy + dz * dz);
    }

    // Match incoming targets to existing tracked targets based on proximity
    const matched = new Set();  // Track which existing targets were matched
    const activeIds = new Set();  // Track which IDs are active this frame

    data.forEach((t, i) => {
        // Find closest existing target
        let closestId = null;
        let closestDist = Infinity;

        Object.keys(targetHistory).forEach(id => {
            if (matched.has(id)) return;  // Already matched this frame
            
            const hist = targetHistory[id];
            const dist = distance3D(t, hist.lastPos);
            
            if (dist < closestDist && dist < TARGET_MATCH_DISTANCE) {
                closestDist = dist;
                closestId = id;
            }
        });

        let targetId;
        if (closestId !== null) {
            // Match found - update existing target
            targetId = closestId;
            matched.add(targetId);
        } else {
            // No match - create new target
            targetId = nextTargetId++;
            
            // Check if we need to remove old targets
            if (Object.keys(targetHistory).length >= MAX_TARGET_HISTORY) {
                removeOldestTarget();
            }

            const col = getTargetColor(targetId);
            targetHistory[targetId] = {
                positions: [],
                lastSeen: Date.now(),
                sphere: null,
                lastPos: { x: t.x, y: t.y, z: t.z },
                color: col,
                velocity: t.v,
                clusterId: t.c
            };
            targetHistoryOrder.push(targetId);

            const mat = new THREE.MeshStandardMaterial({
                color: col,
                emissive: col,
                emissiveIntensity: 0.5
            });
            const sph = new THREE.Mesh(new THREE.SphereGeometry(0.05, 16, 16), mat);
            sph.position.set(t.x, t.z, t.y);
            sph.userData = { x: t.x, y: t.y, z: t.z, v: t.v, c: t.c, id: targetId };
            sceneRoot.add(sph);
            targetHistory[targetId].sphere = sph;
            
            console.log(`Created new target ID ${targetId} at (${t.x.toFixed(2)}, ${t.y.toFixed(2)}, ${t.z.toFixed(2)})`);
        }

        // Update target data
        targetHistory[targetId].sphere.position.set(t.x, t.z, t.y);
        targetHistory[targetId].sphere.userData = { x: t.x, y: t.y, z: t.z, v: t.v, c: t.c, id: targetId };
        targetHistory[targetId].lastPos = { x: t.x, y: t.y, z: t.z };
        targetHistory[targetId].velocity = t.v;
        targetHistory[targetId].clusterId = t.c;

        // Add position to history
        const hist = targetHistory[targetId].positions;
        const shouldAdd = hist.length === 0 ||
            hist[hist.length - 1].x !== t.x ||
            hist[hist.length - 1].y !== t.y ||
            hist[hist.length - 1].z !== t.z;

        const maxLen = Math.min(maxTrailLength, MAX_TRAIL_LENGTH);
        if (shouldAdd) {
            hist.push({ x: t.x, y: t.y, z: t.z });
            while (hist.length > maxLen) {
                hist.shift();
            }
        }
        targetHistory[targetId].lastSeen = Date.now();
        activeIds.add(targetId);
    });

    // Clean up old/inactive targets
    Object.keys(targetHistory).forEach(id => {
        const hist = targetHistory[id];
        const active = activeIds.has(parseInt(id));

        // Remove stale targets (not seen for 5 seconds)
        if (!active && Date.now() - hist.lastSeen > 5000) {
            sceneRoot.remove(hist.sphere);
            delete targetHistory[id];
            const idx = targetHistoryOrder.indexOf(parseInt(id));
            if (idx > -1) targetHistoryOrder.splice(idx, 1);
            return;
        }

        // Remove empty targets
        if (hist.positions.length === 0 && !active) {
            sceneRoot.remove(hist.sphere);
            delete targetHistory[id];
            const idx = targetHistoryOrder.indexOf(parseInt(id));
            if (idx > -1) targetHistoryOrder.splice(idx, 1);
            return;
        }

        // Calculate distance
        const d = (Math.sqrt(
            hist.lastPos.x * hist.lastPos.x +
            hist.lastPos.y * hist.lastPos.y +
            hist.lastPos.z * hist.lastPos.z
        ) * 100).toFixed(0);

        // Create card
        if (hist.sphere) {
            targets.push(hist.sphere);

            const card = document.createElement('div');
            card.className = 'target-card' +
                (active && hist.velocity !== 0 ? ' moving' : '') +
                (!active ? ' inactive' : '');
            card.dataset.targetId = id;
            
            const hexColor = '#' + hist.color.toString(16).padStart(6, '0');
            card.style.borderColor = hexColor;
            card.style.color = hexColor;

            const statusClass = active && hist.velocity !== 0 ? 'moving' : 'still';
            const statusText = active && hist.velocity !== 0 ? '🏃 Moving' : '🧍 Still';

            card.innerHTML = `
                <div class="target-name">Target ${id}${active ? '' : ' (lost)'}</div>
                <div class="target-info">
                    Distance: ${d} cm<br>
                    X: ${(hist.lastPos.x * 100).toFixed(0)} cm<br>
                    Y: ${(hist.lastPos.y * 100).toFixed(0)} cm<br>
                    Z: ${(hist.lastPos.z * 100).toFixed(0)} cm
                </div>
                <div class="target-status ${statusClass}">${statusText}</div>
            `;

            document.getElementById('target-list').appendChild(card);
            targetCards.push(card);
        }
    });

    // Update target count
    document.getElementById('target-panel-count').textContent =
        `${Object.keys(targetHistory).length} target${Object.keys(targetHistory).length === 1 ? '' : 's'}`;

    updateTrails();
}

// ========== TRAIL UPDATES ==========
function updateTrails() {
    trailLines.forEach(l => sceneRoot.remove(l));
    trailLines = [];

    Object.keys(targetHistory).forEach(id => {
        const hist = targetHistory[id].positions;
        if (hist.length < 2) return;

        const points = [];
        hist.forEach(p => points.push(new THREE.Vector3(p.x, p.z, p.y)));

        const curve = new THREE.CatmullRomCurve3(points);
        const geo = new THREE.TubeGeometry(curve, points.length * 2, 0.015, 8, false);
        const col = targetHistory[id].color || 0x00ffff;
        const mat = new THREE.MeshBasicMaterial({
            color: col,
            opacity: 0.7,
            transparent: true
        });
        const tube = new THREE.Mesh(geo, mat);
        sceneRoot.add(tube);
        trailLines.push(tube);
    });
}

function updateTrailLength(val) {
    const clampedVal = Math.min(parseInt(val), MAX_TRAIL_LENGTH);
    maxTrailLength = clampedVal;
    document.getElementById('trail-value').textContent = clampedVal;
    
    Object.keys(targetHistory).forEach(id => {
        while (targetHistory[id].positions.length > clampedVal) {
            targetHistory[id].positions.shift();
        }
    });
    
    updateTrails();
}

// ========== PRESENCE UPDATES ==========
function updatePresence(data) {
    ['zone0', 'zone1', 'zone2', 'zone3'].forEach((id, i) => {
        const e = document.getElementById(id);
        const occ = data[i] === 1;
        e.textContent = occ ? 'Occupied' : 'Empty';
        e.style.color = occ ? '#4caf50' : '#999';
        if (zones[i]) {
            zones[i].material.color.setHex(occ ? 0x4caf50 : 0x4fc3f7);
        }
    });
}

// ========== ANIMATION LOOP ==========
function animate() {
    requestAnimationFrame(animate);

    // Update FPS
    const now = Date.now();
    if (now - stats.lastTime >= 1000) {
        stats.fps = stats.frames;
        stats.frames = 0;
        stats.lastTime = now;
        document.getElementById('fps').textContent = stats.fps;
    }

    renderer.render(scene, camera);
}

// ========== UI CONTROLS ==========
function resetView() {
    camera.position.set(3, 3, 3);
    camera.lookAt(0, 0, 0);
}

function toggleGrid() {
    gridVisible = !gridVisible;
    grid.visible = gridVisible;
    document.getElementById('grid-btn').textContent = 
        gridVisible ? 'Hide Grid' : 'Show Grid';
}

function toggleZones() {
    zonesVisible = !zonesVisible;
    zones.forEach(z => z.visible = zonesVisible);
    detectionZoneMeshes.forEach(m => m.visible = zonesVisible);
    interferenceZoneMeshes.forEach(m => m.visible = zonesVisible);
    zoneLabels.forEach(l => l.visible = zonesVisible);
    document.getElementById('zones-btn').textContent =
        zonesVisible ? 'Hide Zones' : 'Show Zones';
}

// ========== CONFIGURATION CONTROLS ==========
function showFeedback(message, type = 'success') {
    const feedback = document.getElementById('feedback-msg');
    feedback.textContent = message;
    feedback.className = type;
    setTimeout(() => {
        feedback.style.display = 'none';
    }, 3000);
}

async function sendConfigCommand(cmd, value = null) {
    try {
        const payload = value ? { cmd, value } : { cmd };
        const response = await fetch('/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        
        if (response.ok) {
            showFeedback(`✓ ${cmd} command sent`, 'success');
            return true;
        } else {
            showFeedback(`✗ Failed to send ${cmd}`, 'error');
            return false;
        }
    } catch (err) {
        showFeedback(`✗ Error: ${err.message}`, 'error');
        return false;
    }
}

function setSensitivity(value) {
    sendConfigCommand('sensitivity', value);
}

function setTriggerSpeed(value) {
    sendConfigCommand('trigger_speed', value);
}

function resetDetectionZone() {
    sendConfigCommand('reset_detection');
}

function clearInterferenceZone() {
    sendConfigCommand('clear_interference');
}

function autoGenInterference() {
    showFeedback('⚠ Auto-generating interference zones (30-60s)...', 'warning');
    sendConfigCommand('auto_interference');
}

function updateConfigUI(data) {
    // Update UI based on received config
    if (data.sensitivity !== undefined && data.sensitivity !== 255) {
        const levels = ['low', 'medium', 'high'];
        if (data.sensitivity < 3) {
            document.getElementById('sensitivity-select').value = levels[data.sensitivity];
        }
    }
    
    if (data.trigger_speed !== undefined && data.trigger_speed !== 255) {
        const speeds = ['slow', 'medium', 'fast'];
        if (data.trigger_speed < 3) {
            document.getElementById('trigger-select').value = speeds[data.trigger_speed];
        }
    }
}

// ========== 3D ZONE VISUALIZATION ==========
function createZoneBox(bounds, color, opacity) {
    const width = bounds.x_max - bounds.x_min;
    const depth = bounds.y_max - bounds.y_min;
    const height = bounds.z_max - bounds.z_min;
    
    const centerX = (bounds.x_max + bounds.x_min) / 2;
    const centerY = (bounds.y_max + bounds.y_min) / 2;
    const centerZ = (bounds.z_max + bounds.z_min) / 2;
    
    // Create semi-transparent box
    const geometry = new THREE.BoxGeometry(width, height, depth);
    const material = new THREE.MeshBasicMaterial({
        color: color,
        transparent: true,
        opacity: opacity,
        side: THREE.DoubleSide
    });
    const box = new THREE.Mesh(geometry, material);
    
    // Position in scene coordinates (X, Z, Y mapping)
    box.position.set(centerX, centerZ, centerY);
    
    // Create wireframe edges
    const edges = new THREE.EdgesGeometry(geometry);
    const line = new THREE.LineSegments(
        edges,
        new THREE.LineBasicMaterial({ color: color, linewidth: 2 })
    );
    line.position.copy(box.position);
    
    return { box, line };
}

function createZoneLabel(bounds, index, color) {
    const centerX = (bounds.x_max + bounds.x_min) / 2;
    const centerY = (bounds.y_max + bounds.y_min) / 2;
    const centerZ = (bounds.z_max + bounds.z_min) / 2;
    
    const canvas = document.createElement('canvas');
    canvas.width = 512;
    canvas.height = 256;
    const ctx = canvas.getContext('2d');
    
    ctx.fillStyle = '#' + color.toString(16).padStart(6, '0');
    ctx.font = 'bold 32px Arial';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'top';
    
    const text = `Zone ${index}\nX: ${bounds.x_min.toFixed(1)} to ${bounds.x_max.toFixed(1)}m\nY: ${bounds.y_min.toFixed(1)} to ${bounds.y_max.toFixed(1)}m\nZ: ${bounds.z_min.toFixed(1)} to ${bounds.z_max.toFixed(1)}m`;
    const lines = text.split('\n');
    lines.forEach((line, i) => {
        ctx.fillText(line, 256, 16 + i * 40);
    });
    
    const texture = new THREE.CanvasTexture(canvas);
    const spriteMat = new THREE.SpriteMaterial({ map: texture });
    const sprite = new THREE.Sprite(spriteMat);
    sprite.position.set(centerX, centerZ + (bounds.z_max - bounds.z_min) / 2 + 0.3, centerY);
    sprite.scale.set(1, 0.5, 1);
    
    return sprite;
}

function updateDetectionZones(zonesData) {
    // Clear old meshes
    detectionZoneMeshes.forEach(m => {
        sceneRoot.remove(m.box);
        sceneRoot.remove(m.line);
    });
    detectionZoneMeshes = [];
    
    // Create new zone meshes
    zonesData.forEach((zone, i) => {
        const { box, line } = createZoneBox(zone, 0x4fc3f7, 0.15);
        sceneRoot.add(box);
        sceneRoot.add(line);
        detectionZoneMeshes.push({ box, line });
        
        // Create label
        const label = createZoneLabel(zone, i, 0x4fc3f7);
        sceneRoot.add(label);
        zoneLabels.push(label);
    });
    
    console.log('Detection zones updated:', zonesData);
}

function updateInterferenceZones(zonesData) {
    // Clear old meshes
    interferenceZoneMeshes.forEach(m => {
        sceneRoot.remove(m.box);
        sceneRoot.remove(m.line);
    });
    interferenceZoneMeshes = [];
    
    // Create new zone meshes (orange/red color for interference)
    zonesData.forEach((zone, i) => {
        // Only create if zone has non-zero bounds
        if (zone.x_max !== 0 || zone.y_max !== 0 || zone.z_max !== 0) {
            const { box, line } = createZoneBox(zone, 0xff5722, 0.1);
            sceneRoot.add(box);
            sceneRoot.add(line);
            interferenceZoneMeshes.push({ box, line });
        }
    });
    
    console.log('Interference zones updated:', zonesData);
}

// ========== START APPLICATION ==========
init();
