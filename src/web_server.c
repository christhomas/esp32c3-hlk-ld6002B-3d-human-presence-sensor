// Web Server Implementation with Server-Sent Events (SSE) Streaming
// SSE provides real-time updates with ~100ms latency, perfect for 20Hz radar data

#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "WebServer";

// HTTP server handle
static httpd_handle_t server = NULL;

// Client tracking for statistics
static int client_count = 0;

// Shared message buffer for SSE
static char message_buffer[1024];
static SemaphoreHandle_t message_mutex = NULL;

// HTML page with embedded CSS/JS and Three.js (minified)
static const char html_page[] =
"<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>HLK-LD6002B-3D</title>"
"<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:Arial,sans-serif;background:#0a0e27;color:#fff;overflow:hidden}"
"#container{width:100vw;height:100vh;position:relative}#canvas{width:100%;height:100%;display:block}"
"#hud{position:absolute;top:20px;left:20px;background:rgba(0,0,0,0.7);padding:15px;border-radius:8px;font-size:14px;min-width:250px;backdrop-filter:blur(10px)}"
"#hud h2{margin:0 0 10px;font-size:18px;color:#4fc3f7}.status{display:flex;justify-content:space-between;margin:5px 0;padding:5px 0;border-bottom:1px solid rgba(255,255,255,0.1)}"
".label{color:#90caf9}.value{color:#fff;font-weight:bold}.connected{color:#4caf50}.disconnected{color:#f44336}"
"#controls{position:absolute;bottom:20px;right:20px;background:rgba(0,0,0,0.7);padding:15px;border-radius:8px;backdrop-filter:blur(10px)}"
".btn{background:#4fc3f7;border:none;color:#000;padding:8px 16px;margin:5px;border-radius:4px;cursor:pointer;font-size:12px;font-weight:bold}"
".btn:hover{background:#29b6f6}.btn:active{background:#039be5}"
".target-label{position:absolute;background:rgba(0,0,0,0.8);color:#fff;padding:8px 12px;border-radius:6px;font-size:12px;pointer-events:none;"
"border:2px solid #4fc3f7;backdrop-filter:blur(5px);white-space:nowrap;transform:translate(-50%,-120%)}"
".target-label.moving{border-color:#ff9800}.target-label .name{font-weight:bold;color:#4fc3f7;margin-bottom:3px}"
".target-label.moving .name{color:#ff9800}.target-label .info{font-size:10px;color:#90caf9;line-height:1.4}"
"</style></head><body><div id='container'><canvas id='canvas'></canvas>"
"<div id='hud'><h2>🎯 HLK-LD6002B-3D</h2>"
"<div class='status'><span class='label'>Connection:</span><span id='status' class='value disconnected'>Disconnected</span></div>"
"<div class='status'><span class='label'>Targets:</span><span id='target-count' class='value'>0</span></div>"
"<div class='status'><span class='label'>Zone 0:</span><span id='zone0' class='value'>Empty</span></div>"
"<div class='status'><span class='label'>Zone 1:</span><span id='zone1' class='value'>Empty</span></div>"
"<div class='status'><span class='label'>Zone 2:</span><span id='zone2' class='value'>Empty</span></div>"
"<div class='status'><span class='label'>Zone 3:</span><span id='zone3' class='value'>Empty</span></div>"
"<div class='status'><span class='label'>Frames:</span><span id='frame-count' class='value'>0</span></div>"
"<div class='status'><span class='label'>FPS:</span><span id='fps' class='value'>0</span></div></div>"
"<div id='controls'><button class='btn' onclick='resetView()'>Reset View</button>"
"<button class='btn' onclick='toggleGrid()' id='grid-btn'>Hide Grid</button>"
"<button class='btn' onclick='toggleZones()' id='zones-btn'>Hide Zones</button>"
"<div style='margin-top:10px;padding-top:10px;border-top:1px solid rgba(255,255,255,0.2)'>"
"<label style='display:block;margin-bottom:5px;font-size:12px'>Trail Length</label>"
"<input type='range' id='trail-slider' min='0' max='100' value='50' style='width:100%' oninput='updateTrailLength(this.value)'>"
"<div style='text-align:center;font-size:11px;margin-top:3px'><span id='trail-value'>50</span> steps</div>"
"</div></div></div>"
"<script src='https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js'></script>"
"<script>let scene,camera,renderer,grid,zones=[],targets=[],targetLabels=[],stats={frames:0,lastTime:Date.now(),fps:0},gridVisible=true,zonesVisible=true,deviceArrow,sceneRoot;"
"let targetHistory={},trailLines=[],maxTrailLength=50;"
"const targetColors=[0x4fc3f7,0xff9800,0x4caf50,0xe91e63,0x9c27b0,0x00bcd4,0xffeb3b,0xff5722,0x795548,0x607d8b];"
"function getTargetColor(id){return targetColors[parseInt(id)%targetColors.length];}"
"function init(){scene=new THREE.Scene();scene.background=new THREE.Color(0x0a0e27);"
"sceneRoot=new THREE.Group();const rotQuat=new THREE.Quaternion();rotQuat.setFromAxisAngle(new THREE.Vector3(1,0,0),-Math.PI/2);sceneRoot.quaternion.copy(rotQuat);scene.add(sceneRoot);"
"camera=new THREE.PerspectiveCamera(75,window.innerWidth/window.innerHeight,0.1,100);camera.position.set(0,3,-3);camera.lookAt(0,0,0);"
"renderer=new THREE.WebGLRenderer({canvas:document.getElementById('canvas'),antialias:true});"
"renderer.setSize(window.innerWidth,window.innerHeight);renderer.setPixelRatio(window.devicePixelRatio);"
"scene.add(new THREE.AmbientLight(0xffffff,0.6));const dl=new THREE.DirectionalLight(0xffffff,0.8);dl.position.set(5,5,5);scene.add(dl);"
"grid=new THREE.GridHelper(10,20,0x4fc3f7,0x2c3e50);scene.add(grid);scene.add(new THREE.AxesHelper(2));"
"deviceArrow=new THREE.Group();const arrowMat=new THREE.MeshBasicMaterial({color:0xff4444,side:THREE.DoubleSide});"
"const shaftGeo=new THREE.BoxGeometry(0.06,1.2,0.01);const shaft=new THREE.Mesh(shaftGeo,arrowMat);"
"shaft.position.set(0,0.6,0);deviceArrow.add(shaft);"
"const headShape=new THREE.Shape();headShape.moveTo(0,1.3);headShape.lineTo(-0.15,1.15);headShape.lineTo(0.15,1.15);headShape.lineTo(0,1.3);"
"const headGeo=new THREE.ShapeGeometry(headShape);const head=new THREE.Mesh(headGeo,arrowMat);"
"head.position.set(0,0,0.005);deviceArrow.add(head);"
"const canvas=document.createElement('canvas');canvas.width=256;canvas.height=64;const ctx=canvas.getContext('2d');"
"ctx.fillStyle='#ff4444';ctx.font='bold 48px Arial';ctx.textAlign='center';ctx.textBaseline='middle';"
"ctx.fillText('FRONT',128,32);const texture=new THREE.CanvasTexture(canvas);"
"const spriteMat=new THREE.SpriteMaterial({map:texture});const sprite=new THREE.Sprite(spriteMat);"
"sprite.position.set(0,1.5,0);sprite.scale.set(0.5,0.125,1);deviceArrow.add(sprite);sceneRoot.add(deviceArrow);"
"for(let i=0;i<4;i++){const z=new THREE.Mesh(new THREE.BoxGeometry(2,0.05,2),"
"new THREE.MeshBasicMaterial({color:0x4fc3f7,transparent:true,opacity:0.3,wireframe:true}));z.position.set(0,0.5,0);sceneRoot.add(z);zones.push(z);}"
"window.addEventListener('resize',()=>{camera.aspect=window.innerWidth/window.innerHeight;camera.updateProjectionMatrix();"
"renderer.setSize(window.innerWidth,window.innerHeight);});let drag=false,prev={x:0,y:0};"
"renderer.domElement.addEventListener('mousedown',e=>{drag=true;prev={x:e.clientX,y:e.clientY};});"
"renderer.domElement.addEventListener('mousemove',e=>{if(drag){const dx=e.clientX-prev.x,dy=e.clientY-prev.y;"
"camera.position.applyAxisAngle(new THREE.Vector3(0,1,0),-dx*0.005);"
"const r=new THREE.Vector3();r.crossVectors(camera.up,camera.position).normalize();"
"camera.position.applyAxisAngle(r,-dy*0.005);camera.lookAt(0,0,0);prev={x:e.clientX,y:e.clientY};}});"
"renderer.domElement.addEventListener('mouseup',()=>{drag=false;});"
"renderer.domElement.addEventListener('wheel',e=>{e.preventDefault();const dir=camera.position.clone().normalize();"
"camera.position.addScaledVector(dir,e.deltaY>0?0.1:-0.1);});connectSSE();animate();}"
"function connectSSE(){const es=new EventSource('/events');es.onopen=()=>{"
"document.getElementById('status').textContent='Connected';document.getElementById('status').className='value connected';"
"console.log('SSE connected');};"
"es.onerror=()=>{document.getElementById('status').textContent='Reconnecting...';document.getElementById('status').className='value disconnected';"
"setTimeout(()=>connectSSE(),2000);};"
"es.onmessage=e=>{try{const msg=JSON.parse(e.data);stats.frames++;document.getElementById('frame-count').textContent=stats.frames;"
"if(msg.type==='target'){updateTargets(msg.data);document.getElementById('target-count').textContent=msg.data.length;}"
"else if(msg.type==='presence')updatePresence(msg.data);}catch(err){console.error('Parse error:',err);}}; }"
"function updateTargets(data){targetLabels.forEach(l=>l.remove());targetLabels=[];"
"const activeIds=new Set();data.forEach((t,i)=>{activeIds.add(t.c);"
"if(!targetHistory[t.c]){const col=getTargetColor(t.c);targetHistory[t.c]={positions:[],lastSeen:Date.now(),sphere:null,lastPos:{x:t.x,y:t.y,z:t.z},color:col};"
"const mat=new THREE.MeshStandardMaterial({color:col,emissive:col,emissiveIntensity:0.5});"
"const sph=new THREE.Mesh(new THREE.SphereGeometry(0.05,16,16),mat);sph.position.set(t.x,t.z,t.y);sph.userData={x:t.x,y:t.y,z:t.z,v:t.v,c:t.c};"
"sceneRoot.add(sph);targets.push(sph);targetHistory[t.c].sphere=sph;}else{"
"targetHistory[t.c].sphere.position.set(t.x,t.z,t.y);targetHistory[t.c].sphere.userData={x:t.x,y:t.y,z:t.z,v:t.v,c:t.c};"
"targetHistory[t.c].lastPos={x:t.x,y:t.y,z:t.z};}"
"const hist=targetHistory[t.c].positions;const shouldAdd=hist.length===0||hist[hist.length-1].x!==t.x||hist[hist.length-1].y!==t.y||hist[hist.length-1].z!==t.z;"
"if(shouldAdd){hist.push({x:t.x,y:t.y,z:t.z});if(hist.length>maxTrailLength)hist.shift();}targetHistory[t.c].lastSeen=Date.now();});"
"Object.keys(targetHistory).forEach(id=>{const hist=targetHistory[id];const d=(Math.sqrt(hist.lastPos.x*hist.lastPos.x+hist.lastPos.y*hist.lastPos.y+hist.lastPos.z*hist.lastPos.z)*100).toFixed(0);"
"const active=activeIds.has(parseInt(id));const lbl=document.createElement('div');lbl.className='target-label'+(active&&hist.sphere.userData.v!==0?' moving':'');"
"if(!active)lbl.style.opacity='0.5';"
"lbl.innerHTML=`<div class='name'>Target ${id}${active?'':' (lost)'}</div><div class='info'>Dist: ${d}cm | ${active&&hist.sphere.userData.v!==0?'Moving':'Still'}<br>`+"
"`X: ${(hist.lastPos.x*100).toFixed(0)}cm | Y: ${(hist.lastPos.y*100).toFixed(0)}cm | Z: ${(hist.lastPos.z*100).toFixed(0)}cm</div>`;"
"document.getElementById('container').appendChild(lbl);targetLabels.push(lbl);});updateTrails();}"
"function updateTrails(){trailLines.forEach(l=>sceneRoot.remove(l));trailLines=[];"
"Object.keys(targetHistory).forEach(id=>{const hist=targetHistory[id].positions;if(hist.length<2)return;"
"const points=[];hist.forEach(p=>points.push(new THREE.Vector3(p.x,p.z,p.y)));"
"const curve=new THREE.CatmullRomCurve3(points);const geo=new THREE.TubeGeometry(curve,points.length*2,0.015,8,false);"
"const col=targetHistory[id].color||0x00ffff;const mat=new THREE.MeshBasicMaterial({color:col,opacity:0.7,transparent:true});"
"const tube=new THREE.Mesh(geo,mat);sceneRoot.add(tube);trailLines.push(tube);});}"
"function updateTrailLength(val){maxTrailLength=parseInt(val);document.getElementById('trail-value').textContent=val;"
"Object.keys(targetHistory).forEach(id=>{while(targetHistory[id].positions.length>maxTrailLength)targetHistory[id].positions.shift();});updateTrails();}"
"function updatePresence(data){['zone0','zone1','zone2','zone3'].forEach((id,i)=>{"
"const e=document.getElementById(id),occ=data[i]===1;e.textContent=occ?'Occupied':'Empty';e.style.color=occ?'#4caf50':'#999';"
"if(zones[i])zones[i].material.color.setHex(occ?0x4caf50:0x4fc3f7);});}"
"function animate(){requestAnimationFrame(animate);const now=Date.now();if(now-stats.lastTime>=1000){"
"stats.fps=stats.frames;stats.frames=0;stats.lastTime=now;document.getElementById('fps').textContent=stats.fps;}"
"targets.forEach((t,i)=>{if(targetLabels[i]){const pos=t.position.clone().project(camera);"
"const x=(pos.x*0.5+0.5)*window.innerWidth;const y=(-pos.y*0.5+0.5)*window.innerHeight;"
"targetLabels[i].style.left=x+'px';targetLabels[i].style.top=y+'px';}});"
"renderer.render(scene,camera);}"
"function resetView(){camera.position.set(3,3,3);camera.lookAt(0,0,0);}"
"function toggleGrid(){gridVisible=!gridVisible;grid.visible=gridVisible;document.getElementById('grid-btn').textContent=gridVisible?'Hide Grid':'Show Grid';}"
"function toggleZones(){zonesVisible=!zonesVisible;zones.forEach(z=>z.visible=zonesVisible);"
"document.getElementById('zones-btn').textContent=zonesVisible?'Hide Zones':'Show Zones';}init();</script></body></html>";

// Root handler
static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
}

// SSE handler - streams events to connected clients
static esp_err_t sse_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/event-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    client_count++;
    ESP_LOGI(TAG, "SSE client connected (%d total)", client_count);
    
    // Send initial ping
    const char *init = "data: {\"type\":\"connected\"}\n\n";
    if (httpd_resp_send_chunk(req, init, strlen(init)) != ESP_OK) {
        client_count--;
        return ESP_FAIL;
    }
    
    // Keep connection alive and send data
    char last_msg[1024] = {0};
    for (int i = 0; i < 36000; i++) {  // Max 1 hour (36000 * 100ms)
        // Check if there's a new message
        if (message_mutex && xSemaphoreTake(message_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (strlen(message_buffer) > 0 && strcmp(message_buffer, last_msg) != 0) {
                // Format as SSE
                char sse_msg[1100];
                snprintf(sse_msg, sizeof(sse_msg), "data: %s\n\n", message_buffer);
                
                if (httpd_resp_send_chunk(req, sse_msg, strlen(sse_msg)) != ESP_OK) {
                    xSemaphoreGive(message_mutex);
                    break;
                }
                strncpy(last_msg, message_buffer, sizeof(last_msg) - 1);
            }
            xSemaphoreGive(message_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Poll every 10ms for low latency
    }
    
    client_count--;
    ESP_LOGI(TAG, "SSE client disconnected (%d remaining)", client_count);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t web_server_init(void) {
    message_mutex = xSemaphoreCreateMutex();
    if (!message_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_FAIL;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_open_sockets = 4;  // Max 7 allowed (3 used internally), using 4 for clients
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;
    
    ESP_LOGI(TAG, "Starting web server");
    
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server");
        return ESP_FAIL;
    }
    
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &root_uri);
    
    httpd_uri_t sse_uri = {
        .uri = "/events",
        .method = HTTP_GET,
        .handler = sse_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &sse_uri);
    
    ESP_LOGI(TAG, "✅ Web server started with SSE streaming");
    return ESP_OK;
}

void web_server_deinit(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    if (message_mutex) {
        vSemaphoreDelete(message_mutex);
        message_mutex = NULL;
    }
    ESP_LOGI(TAG, "Web server stopped");
}

bool web_server_is_running(void) {
    return server != NULL;
}

// Helper to queue message for SSE broadcast
static void queue_message(const char *json) {
    if (!message_mutex || !json) return;
    
    if (xSemaphoreTake(message_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        strncpy(message_buffer, json, sizeof(message_buffer) - 1);
        message_buffer[sizeof(message_buffer) - 1] = '\0';
        xSemaphoreGive(message_mutex);
    }
}

void web_server_send_targets(const radar_target_t* targets, int32_t target_count) {
    if (!server || client_count == 0) return;
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "target");
    
    cJSON *data = cJSON_CreateArray();
    for (int i = 0; i < target_count; i++) {
        cJSON *target = cJSON_CreateObject();
        cJSON_AddNumberToObject(target, "x", targets[i].x);
        cJSON_AddNumberToObject(target, "y", targets[i].y);
        cJSON_AddNumberToObject(target, "z", targets[i].z);
        cJSON_AddNumberToObject(target, "v", targets[i].velocity);
        cJSON_AddNumberToObject(target, "c", targets[i].cluster_id);
        cJSON_AddItemToArray(data, target);
    }
    cJSON_AddItemToObject(root, "data", data);
    
    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        queue_message(json);
        free(json);
    }
    cJSON_Delete(root);
}

void web_server_send_point_cloud(int32_t point_count, const float* points, int max_points) {
    // Point cloud support (optional - can be enabled later)
    (void)point_count;
    (void)points;
    (void)max_points;
}

void web_server_send_presence(uint32_t zone0, uint32_t zone1, 
                              uint32_t zone2, uint32_t zone3) {
    if (!server || client_count == 0) return;
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "presence");
    
    cJSON *data = cJSON_CreateArray();
    cJSON_AddItemToArray(data, cJSON_CreateNumber(zone0));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(zone1));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(zone2));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(zone3));
    cJSON_AddItemToObject(root, "data", data);
    
    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        queue_message(json);
        free(json);
    }
    cJSON_Delete(root);
}

void web_server_send_config(uint8_t sensitivity, uint8_t trigger_speed, 
                            uint8_t install_method) {
    if (!server || client_count == 0) return;
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "config");
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "sensitivity", sensitivity);
    cJSON_AddNumberToObject(data, "trigger_speed", trigger_speed);
    cJSON_AddNumberToObject(data, "install_method", install_method);
    cJSON_AddItemToObject(root, "data", data);
    
    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        queue_message(json);
        free(json);
    }
    cJSON_Delete(root);
}

int web_server_get_client_count(void) {
    return client_count;
}
