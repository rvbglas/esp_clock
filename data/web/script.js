var pages
var parameters = {}
var daynames = ['Пн', 'Вт', 'Ср', 'Чт', 'Пт', 'Сб', 'Вс'] 
var msgT

function toggleMenu(e) {
  active = (document.getElementById('menuLink').className.indexOf('active') !== -1)
  if (active || e.target.id == 'menuLink' || e.target.id == 'menuBtn') {
    elements = [ document.getElementById('layout'), document.getElementById('menu'), document.getElementById('menuLink') ]
    for (const element of elements) {
      if (!active) {
        element.classList.add('active')
      } else {
        element.classList.remove('active')
      }
    }
    if (e.target.id == 'menuLink' || e.target.id == 'menuBtn') {
      e.preventDefault()
    }
    e.stopPropagation()
  }
}

document.getElementById('layout').addEventListener('click', toggleMenu)

function encode(r){
  r = String(r)
  return r.replace(/[\x26\x0A\<>'"]/g,function(r){return"&#"+r.charCodeAt(0)+";"})
}

function getAnchor() {
    return window.location.hash.slice(1);
}

function drawHeader(project) {
  var menu_header = document.getElementById('_ui_menu_header')
  menu_header.innerHTML = project.name
  document.title = project.name + '/' + project.version
}

function parseJsonQ(url, callback) {
  var req = new XMLHttpRequest();

  req.onreadystatechange = function () {
    if (this.readyState != 4) return; 
    if (this.status != 200 && this.status != 500 && this.status != 404) {
      setTimeout(parseJsonQ(url, callback),30000);
      return;
    }
    var json = JSON.parse(this.responseText)
    callback(json);
  };

  req.open("GET", url, true);
  req.send()

}

function updateElement(id, value) {
  var element = document.getElementById("_ui_element_"+id)
  if (!element) return;
  var ui_class = element.dataset.ui_class;
  switch (ui_class) {
    case "table":
      element.innerText = value
      break
    case "input":
    case "password":
    case "select":
    case "number":
    case "range":
      element.value = value  
      break
    case "checkbox":
      element.checked = value
      break
    case "week":
      for (i=0; i<7; i++) {
        var subname = '_ui_elpart_'+i+'_'+id
        var subelement = document.getElementById(subname) 
        if (value[i]==1) { 
          subelement.className = "weekday-selected"
        } else {
          subelement.className = "weekday"
        }
      }
      break
  }  
}

function updateValues(json) {
  for (var key in json) {
    var obj = document.getElementById("_ui_element_"+key)
    if (obj) {
      updateElement(key, json[key])
    }
    parameters[key] = json[key]
  }
  var notification = document.getElementById('_ui_notification');
  if (parameters['_changed']) {
    notification.innerHTML = '<input type="button" id="save" value="Сохранить" class="pure-button" onclick="sendAction(\'save\')">'
    notification.removeAttribute('hidden')
  } else {
    notification.innerHTML = ''
    notification.hidden = true
  }
}

function sendUpdate(id) {
  var input = document.getElementById('_ui_element_'+id)
  var ui_class = input.dataset.ui_class;
  switch (ui_class) {
    case 'input':
    case 'password':
    case 'number':
    case 'range':
      if (input.checkValidity() && input.value != parameters[id]) {
        parseJsonQ('/config/set?name=' + id + '&value=' + encodeURIComponent(input.value), function(json) {
          updateValues(json)
        })
      }
      break
    case 'select':
      parseJsonQ('/config/set?name=' + id + '&value=' + encodeURIComponent(input.selectedOptions[0].value), function(json) {
        updateValues(json)
      })
      break;
    case 'checkbox':
      parseJsonQ('/config/set?name=' + id + '&value=' + (input.checked?'true':'false'), function(json) {
        updateValues(json)
      })      
      break;
    case 'week':        
      parseJsonQ('/config/set?name=' + id + '&value=' + input.dataset.value, function(json) {
        updateValues(json)
      })      
      break;
  }
}

function sendAction(name, params = {}) {
  var url = '/action?name=' + name
  for (var param in params) {
    url += '&'+param+'='+encodeURIComponent(params[param])
  }
  parseJsonQ(url, function(json) {
    if (json.result == 'FAILED') {
      alert(json.message)
      if (json.page) {
        drawPage(json.page)
      }
    } else {
      location.reload()
    }
  })
}

function showPwd(id) {
  var x = document.getElementById('_ui_element_' + id)
  if (x.type === "password") {
    x.type = "text";
  } else {
    x.type = "password";
  }
}

function openSelect(id) {
  var selector = document.getElementById('_ui_elemmodal_'+id);
  selector.removeAttribute("hidden")
}

function closeSelect(id) {
  var selector = document.getElementById('_ui_elemmodal_'+id);
  selector.hidden = true
}

function closeMsg() {
  document.getElementById("_ui_message").hidden = true;
}

function fadeMsg() {
  var msg = document.getElementById('_ui_message');
  msg.classList.add("fadeout")
  msgT = setTimeout(()=> { closeMsg() }, 5000);
}

function openMsg(msgText) {
  document.getElementById("_ui_message_text").innerText = msgText;
  document.getElementById("_ui_message").classList.remove("fadeout"); 
  document.getElementById("_ui_message").removeAttribute('hidden');
  
  if (msgT) { 
    window.clearTimeout(msgT); 
  }
  msgT = setTimeout(()=> { fadeMsg(); }, 5000);
}

function selectWiFi(id, ssid) {
  closeSelect(id);
  var x = document.getElementById('_ui_element_' + id)
  updateElement(x,ssid);
  sendUpdate(id)
}

function getWiFi(id) {
  var list = document.getElementById('_ui_elemselect_'+id)

  var req = new XMLHttpRequest();

  req.onreadystatechange = function () {
    if (this.readyState != 4) return; 
    if (this.status != 200 && this.status != 500 && this.status != 404) {
      setTimeout(getWiFi(id),30000);
      return;
    }
    var json = JSON.parse(this.responseText)
    var table = '<table cellpadding="5" border="0" align="center"><thead class="table-header"><tr><td>SSID</td><td>BSSID</td><td>RSSI</td><td>Канал</td><td>Защита</td></tr></thead><tbody>'
    if (!json.length) {
      setTimeout(getWiFi(id),5000);
    }
    for (idx in json) {
      var encryption = json[idx].secure == 2? "TKIP" : json[idx].secure == 5? "WEP" : json[idx].secure == 4? "CCMP" : json[idx].secure == 7? "нет" : json[idx].secure == 8? "Автоматически" : "Не определено";
      table += '<tr onclick="selectWiFi(\''+id+'\',\''+json[idx].ssid+'\')"><td>'+json[idx].ssid+'</td><td>'+json[idx].bssid+'</td><td>'+json[idx].rssi+'</td><td>'+json[idx].channel+'</td><td>'+encryption+'</td></tr>'
    }
    
    table += '</tbody></table>'
    list.innerHTML = table;
  };

  req.open("GET", "/wifi/scan", true);
  req.send()
}

function clickDay(id, day) {
  value = parameters[id].split('')
  day_value = value[day]
  day_value = (day_value=='0')?'1':'0'
  value[day] = day_value
  element = document.getElementById('_ui_element_' + id)
  value = value.join('')
  element.dataset.value = value;
  sendUpdate(id);
}

function sendTime(id) {
  var value = document.getElementById('_ui_element_' + id).value
  if (value) {
    var date = new Date(value)
    var timestamp = Math.floor(date.getTime()/1000);
    sendAction('time',{"timestamp":timestamp})
  }
}

function elementHTML(element) {
  var value
  if (parameters[element.id] || !isNaN(parameters[element.id])) {
    value = parameters[element.id]
  } else if (element.value) {
    value = element.value
  } else {
    value = ""
  }
  switch (element.type) {
    case 'hr':
      return '<div class="pure-u-1 pure-u-md-1-3"><hr></div>'
    case 'button':
      return '<div class="pure-u-1 pure-u-md-1-3"><div align="center"><input type="button" id="' 
        + element.id + '" value="' + encode(element.label) + '" class="pure-button" onclick="sendAction(\'' + element.id + '\')" /></div></div>'
    case 'password':
      return '<div class="pure-u-1 pure-u-md-1-3"><label for="_ui_element_' + element.id + '">' + encode(element.label) + '</label>'
        + '<div class="hinted"><input data-ui_class="password" type="password" id="_ui_element_' + element.id + '" value="' + encode(value) 
        + '" class="pure-u-1" maxlength="99" oninput="sendUpdate(\'' + element.id + '\')" />'
        + '<z class="hint" onclick="showPwd(\'' + element.id + '\')">&#61550;</z></div></div>'
    case 'input':
      var pattern = ""
      if (element.pattern) {
        pattern = ' pattern="' + encode(element.pattern) + '"'
      }
      return '<div class="pure-u-1 pure-u-md-1-3"><label for="_ui_element_' + element.id + '">' + encode(element.label) + '</label>'
        + '<input data-ui_class="input" type="text" id="_ui_element_' + element.id + '" value="' + encode(value) +'"'+ pattern
        + ' class="pure-u-1" maxlength="99" oninput="sendUpdate(\'' + element.id + '\')" /></div>'
    case 'input-wifi':
      var pattern = ""
      if (element.pattern) {
        pattern = ' pattern="' + encode(element.pattern) + '"'
      }
      return '<div class="pure-u-1 pure-u-md-1-3"><label for="_ui_element_' + element.id + '">' + encode(element.label) + '</label>'
        + '<div class="hinted"><input data-ui_class="input" type="text" id="_ui_element_' + element.id + '" value="' + encode(value) +'"'+ pattern
        + ' class="pure-u-1" maxlength="99" oninput="sendUpdate(\'' + element.id + '\')" />' 
        + '<z class="hint" onclick="openSelect(\'' + element.id+ '\'); getWiFi(\'' + element.id + '\')">&#61931;</z></div>'
        + '<div class="modal" id="_ui_elemmodal_' + element.id + '" hidden>' 
        + '<div class="modal-content">'
        + '<div id="_ui_elemselect_' + element.id + '"></div>'
        + '<div class="pure-u-1 pure-u-md-1-3"><div align="center"><input type="button" id="_ui_button_' 
        + element.id + '" value="Закрыть" class="pure-button" onclick="closeSelect(\'' + element.id + '\')"></div>'
        + '</div></div></div></div>'
    case 'checkbox':
      return '<div class="pure-u-1 pure-u-md-1-3"><label class="switch socket" for="_ui_element_' + element.id + '">'
        + '<input class="switch" data-ui_class="checkbox" type="checkbox" id="_ui_element_' + element.id + '"' + (parameters[element.id]?' checked':'') + ' onchange="sendUpdate(\'' + element.id + '\')" />'
        + '<span class="switch slider">'+ encode(element.label) + '</span>'
        + '</label></div>'
    case 'select':
      var options = '<div class="pure-u-1 pure-u-md-1-3"><label for="_ui_element_' + element.id + '">' + encode(element.label) + '</label>'
        + '<select class="pure-u-24-24" data-ui_class="select" id="_ui_element_' + element.id + '" onchange="sendUpdate(\'' + element.id + '\')">'
      for (const option of element.options) {
        var list_option = '<option value="' + encode(option.value) + '" ';
        if (option.value == parameters[element.id]) {
          list_option += 'selected '
        }
        list_option += '>' + encode(option.text) + '</option>'
        options += list_option
      }
      options += '</select></div>'
      return options
    case 'week':
      days = '<div class="pure-u-1 pure-u-md-1-3"><label for="_ui_element_' + element.id + '">' + encode(element.label) + '</label>' 
        + '<table data-ui_class="week" data-value="' + value + '" id="_ui_element_' + element.id + '" cellpadding="5" border="0" class="week"><tbody><tr>'
      for (i=0; i<7; i++) {
        a_enabled = (value[i] == "1")
        days = days + '<td><div class="weekday' + (a_enabled?"-selected":"") + '" id="_ui_elpart_'+i+'_'+element.id+'" onclick="clickDay(\'' + element.id + '\', ' + i + ')">'+ daynames[i] + '</div></td>'
      }
      days += '</tr></tbody></table></div>'
      return days
    case 'timeset':
      var now = new Date()
      now.setMinutes(now.getMinutes() - now.getTimezoneOffset())
      value = now.toISOString().slice(0, -1);
      return '<div class="pure-u-1 pure-u-md-1-3"><label for="_ui_element_' + element.id + '">' + encode(element.label) + '</label>'
        +'<div class="timesetter"><input id="_ui_element_'+element.id+'" data-ui_class="timeset" class="inline-input" type="datetime-local" value="'+value+'">'
        + '<div class="send-button" onclick="sendTime(\''+element.id+'\')">-></div></div></div>'
    case 'text':
      return '<div class="pure-u-1 pure-u-md-1-3"><h2 id="_ui_element_'+ element.id +'" ' + (element.color?'style="color:'+ element.color+'" ':'')+ '>' + encode(value) + '</h2></div>'
    case 'number':
      return '<div class="pure-u-1 pure-u-md-1-3"><label for="_ui_element_' + element.id + '">' + encode(element.label) + '</label>'
        + '<input data-ui_class="number" type="number" '+ (!isNaN(element.min)?'min="'+element.min+'" ':'') + (!isNaN(element.max)?'max="'+element.max+'" ':'') + (!isNaN(element.step)?'step="'+element.step+'" ':'')
        + 'id="_ui_element_' + element.id + '" value="' + encode(value) +'"'+ pattern
        + ' class="pure-u-1" maxlength="99" oninput="sendUpdate(\'' + element.id + '\')" /></div>'
    case 'range':
      return '<div class="pure-u-1 pure-u-md-1-3"><label for="_ui_element_' + element.id + '">' + encode(element.label) + '</label>'
        + '<input data-ui_class="range" type="range" '+ (!isNaN(element.min)?'min="'+element.min+'" ':'') + (!isNaN(element.max)?'max="'+element.max+'" ':'') + (!isNaN(element.step)?'step="'+element.step+'" ':'')
        + 'id="_ui_element_' + element.id + '" value="' + encode(value) +'"'+ pattern
        + ' class="pure-u-1" maxlength="99" oninput="sendUpdate(\'' + element.id + '\')" /></div>'
    case 'table':
    default:
      return '<div class="pure-u-1 pure-u-md-1-3"><table class="texttable" cellpadding="5" border="0" align="center"><tbody><tr><td class="value-name" align="right">' 
        + encode(element.label)+ '</td><td id="_ui_element_' 
        + element.id + '" data-ui_class="table" ' + (element.color?'style="color:'+ element.color+'" ':'')+ '>' + encode(value) + '</td></tr></tbody></table></div>'
  }
}

function drawPage(id) {
  var idx =0, i=0
  for (const page of pages) {
    var menu_link = document.getElementById('_ui_pglink_' + page.id)  
    if (page.id != id) {
      menu_link.classList.remove("pure-menu-selected")
    } else {
      menu_link.classList.add("pure-menu-selected")
      idx = i
    }
    i++
  }
  var page_header = document.getElementById("_ui_page_header")
  page_header.innerHTML = pages[idx].title
  var page_content = document.getElementById("_ui_page_content");
  var content = ''
  for (const element of pages[idx].elements) {
    content = content + elementHTML(element) + '\n'
  }
  page_content.innerHTML = content
  window.location.hash = id
}

function drawNavigator(project, pages) {
  var menu = document.getElementById('_ui_menu_list');
  var list = ''
  for (const page of pages) {
    list = list + '<li id="_ui_pglink_' + page.id 
      + '" class="pure-menu-item"><a class="pure-menu-link" onclick="drawPage(\''+ page.id +'\')" href="#' + page.id + '">'
      + (page.icon?'<span class="icon">'+page.icon+'</span>':'')
      + page.title+'</a></li>'
  }
  menu.innerHTML = list
}

function drawContacts(contacts) {
  if (!contacts) return;
  var contact_list = '<hr><h4 class="pure-u1">Контакты</h4>'
  for (contact of contacts) {
    const url = new URL(contact)
    var ref
    switch (url.protocol) {
      case 'http':
      case 'https:':
        ref = '<span class="icon">&#61461;</span>'+url.hostname
        break
      case 'mailto:':
        ref = '<span class="icon">&#61664;</span>'+url.pathname
        break
      case 'tg:':
        ref = '<span class="icon">&#62150;</span>'+url.pathname
        contact = 'tg://resolve?domain='+url.pathname
        break
      default:
        ref = '<span class="icon">&#62074;</span>'+url.pathname
    }
    contact_list += '<a href="'+contact+'">'+ref+'</a>'
  }
  var footer = document.getElementById('_ui_contacts');
  footer.innerHTML = contact_list
}

function drawUI(ui) {
  drawHeader(ui.project)
  pages = ui.pages
  drawNavigator(ui.project, pages)
  drawContacts(ui.project.contacts)
  var anchor = getAnchor()
  if (anchor) {
    drawPage(anchor)
  } else {
    drawPage(pages[0].id)
  }
}

function GetUI() {
  parseJsonQ("/ui", drawUI);
}

GetUI()

function initES() {
  if (!!window.EventSource) {
    var source = new EventSource('/events');

    source.onerror = function(e) {
      if (source.readyState == 2) {
        setTimeout(initES, 5000);
      }
    };
        
    source.addEventListener('update', function(e) {
      updateValues(JSON.parse(e.data));
    }, false);
    source.addEventListener('message', function(e) {
      openMsg(e.data);
    }, false);
  }
}

initES();

function drawConfig(cfg) {
  updateValues(cfg)
}

function GetCfg() {
  parseJsonQ("/config/get", drawConfig);
}

GetCfg()
