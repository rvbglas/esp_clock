var pages
var parameters = {}
var daynames = ['Пн', 'Вт', 'Ср', 'Чт', 'Пт', 'Сб', 'Вс'] 

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
    return window.location.hash;
}

function DrawHeader(project) {
  var menu_header = document.getElementById('_ui_menu_header')
  menu_header.innerHTML = project.name
  document.title = project.name + '/' + project.version
}

function ParseJsonQ(url, callback) {
  var req = new XMLHttpRequest();

  req.onreadystatechange = function () {
    if (this.readyState != 4) return; 
    if (this.status != 200 && this.status != 500 && this.status != 404) {
      setTimeout(ParseJsonQ(url, callback),30000);
      return;
    }
    var json = JSON.parse(this.responseText)
    callback(json);
  };

  req.open("GET", url, true);
  req.send()

}

function UpdateElement(id, value) {
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

function UpdateValues(json) {
  for (var key in json) {
    var obj = document.getElementById("_ui_element_"+key)
    if (obj) {
      UpdateElement(key, json[key])
    }
    parameters[key] = json[key]
  }
  var notification = document.getElementById('_ui_notification');
  if (parameters['_changed']) {
    notification.innerHTML = '<input style="margin:1em .5em 0;" type="button" id="save" value="Сохранить внесенные изменения" class="pure-button pure-button-primary" onclick="sendAction(\'save\')">'
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
        ParseJsonQ('/config/set?name=' + id + '&value=' + encodeURIComponent(input.value), function(json) {
          UpdateValues(json)
        })
      }
      break
    case 'select':
      ParseJsonQ('/config/set?name=' + id + '&value=' + encodeURIComponent(input.selectedOptions[0].value), function(json) {
        UpdateValues(json)
      })
      break;
    case 'checkbox':
      ParseJsonQ('/config/set?name=' + id + '&value=' + (input.checked?'true':'false'), function(json) {
        UpdateValues(json)
      })      
      break;
    case 'week':        
      ParseJsonQ('/config/set?name=' + id + '&value=' + input.dataset.value, function(json) {
        UpdateValues(json)
      })      
      break;
  }
}

function sendAction(name) {
  ParseJsonQ('/action?name=' + name, function(json) {
    if (json.result == 'FAILED') {
      alert(json.message)
      if (json.page) {
        DrawPage(json.page)
      }
    } else {
      location.reload()
    }
  })
}

function ShowPwd(id) {
  var x = document.getElementById('_ui_element_' + id)
  if (x.type === "password") {
    x.type = "text";
  } else {
    x.type = "password";
  }
}

function OpenSelect(id) {
  var selector = document.getElementById('_ui_elemmodal_'+id);
  selector.removeAttribute("hidden")
}

function CloseSelect(id) {
  var selector = document.getElementById('_ui_elemmodal_'+id);
  selector.hidden = true
}

function SelectWiFi(id, ssid) {
  CloseSelect(id);
  var x = document.getElementById('_ui_element_' + id)
  UpdateElement(x,ssid);
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
    var table = '<table cellpadding="5" border="0" align="center"><thead class="table-header"><tr><td style="padding: 1rem">SSID</td><td style="padding: 1rem">BSSID</td><td style="padding: 1rem">RSSI</td><td style="padding: 1rem">Канал</td><td style="padding: 1rem">Защита</td></tr></thead></tbody style="border-bottom: lightgrey 1px solid">'
    if (!json.length) {
      setTimeout(getWiFi(id),5000);
    }
    for (idx in json) {
      var encryption = json[idx].secure == 2? "TKIP" : json[idx].secure == 5? "WEP" : json[idx].secure == 4? "CCMP" : json[idx].secure == 7? "нет" : json[idx].secure == 8? "Автоматически" : "Не определено";
      table += '<tr onclick="SelectWiFi(\''+id+'\',\''+json[idx].ssid+'\')"><td style="padding: 1rem">'+json[idx].ssid+'</td><td style="padding: 1rem">'+json[idx].bssid+'</td><td style="padding: 1rem">'+json[idx].rssi+'</td><td style="padding: 1rem">'+json[idx].channel+'</td><td style="padding: 1rem">'+encryption+'</td></tr>'
    }
    
    table += '</tbody></table>'
    list.innerHTML = table;
  };

  req.open("GET", "/wifi/scan", true);
  req.send()

}

function ClickDay(id, day) {
  value = parameters[id].split('')
  day_value = value[day]
  day_value = (day_value=='0')?'1':'0'
  value[day] = day_value
  element = document.getElementById('_ui_element_' + id)
  value = value.join('')
  element.dataset.value = value;
  sendUpdate(id);
}

function ElementHTML(element) {
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
      return '<div class="pure-u-1 pure-u-md-1-3"><div align="center"><input style="margin:1em .5em 0;" type="button" id="' 
        + element.id + '" value="' + encode(element.label) + '" class="pure-button pure-button-primary" onclick="sendAction(\'' + element.id + '\')" /></div></div>'
    case 'password':
      return '<div class="pure-u-1 pure-u-md-1-3"><label for="_ui_element_' + element.id + '">' + encode(element.label) + '</label>'
        + '<input data-ui_class="password" type="password" id="_ui_element_' + element.id + '" value="' + encode(value) 
        + '" class="pure-u-1" style="display:inline-block" maxlength="99" oninput="sendUpdate(\'' + element.id + '\')" />'
        + '<div class="hint" onclick="ShowPwd(\'' + element.id + '\')">&#x1F441;</div></div>'
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
        + '<input data-ui_class="input" type="text" id="_ui_element_' + element.id + '" value="' + encode(value) +'"'+ pattern
        + ' class="pure-u-1" style="display:inline-block" maxlength="99" oninput="sendUpdate(\'' + element.id + '\')" />' 
        + '<div class="hint" onclick="OpenSelect(\'' + element.id+ '\'); getWiFi(\'' + element.id + '\')">&#128246;</div>'
        + '<div class="modal" id="_ui_elemmodal_' + element.id + '" hidden>' 
        + '<div class="modal-content">'
        + '<div id="_ui_elemselect_' + element.id + '"></div>'
        + '<div class="pure-u-1 pure-u-md-1-3"><div align="center"><input style="margin:1em .5em 0;" type="button" id="_ui_button_' 
        + element.id + '" value="Закрыть" class="pure-button pure-button-primary" onclick="CloseSelect(\'' + element.id + '\')"></div>'
        + '</div></div></div>'
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
        days = days + '<td><div class="weekday' + (a_enabled?"-selected":"") + '" id="_ui_elpart_'+i+'_'+element.id+'" onclick="ClickDay(\'' + element.id + '\', ' + i + ')">'+ daynames[i] + '</div></td>'
      }
      days += '</tr></tbody></table></div>'
      return days
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
      return '<div class="pure-u-1 pure-u-md-1-3"><table cellpadding="5" border="0" align="center"><tbody><tr><td style="padding-right: 10px; width: 50%;" align="right"><pre>' 
        + encode(element.label)+ '</pre></td><td style="padding-left: 10px;" id="cT"><pre id="_ui_element_' 
        + element.id + '" data-ui_class="table" ' + (element.color?'style="color:'+ element.color+'" ':'')+ '>' + encode(value) + '</pre></td></tr></tbody></table></div>'
  }
}

function DrawPage(id) {
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
    content = content + ElementHTML(element) + '\n'
  }
  page_content.innerHTML = content
  window.location.hash = id
}

function DrawNavigator(project, pages) {
  var menu = document.getElementById('_ui_menu_list');
  var list = ''
  for (const page of pages) {
    list = list + '<li id="_ui_pglink_' + page.id 
      + '" class="pure-menu-item"><a class="pure-menu-link" onclick="DrawPage(\''+ page.id +'\')" href="#' + page.id + '">'
      + page.title+'</a></li>'
  }
  menu.innerHTML = list
}

function DrawContacts(contacts) {
  if (!contacts) return;
  var contact_list = '<hr><h4 class="pure-u1">Контакты</h4>'
  for (const contact of contacts) {
    const url = new URL(contact)
    var ref
    console.log(url)
    switch (url.protocol) {
      case 'http':
      case 'https:':
        ref = '&#8962; '+url.hostname
        break
      case 'mailto:':
        ref = '&#9993; '+url.pathname
        break
      default:
        ref = '&#128389; '+url.pathname
    }
    contact_list += '<a href="'+contact+'">'+ref+'</a>'
  }
  var footer = document.getElementById('_ui_contacts');
  footer.innerHTML = contact_list
}

function DrawUI(ui) {
  DrawHeader(ui.project)
  pages = ui.pages
  DrawNavigator(ui.project, pages)
  DrawContacts(ui.project.contacts)
  var anchor = getAnchor()
  if (anchor) {
    DrawPage(anchor)
  } else {
    DrawPage(pages[0].id)
  }
}

function GetUI() {
  ParseJsonQ("/ui", DrawUI);
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
        
    source.addEventListener('keepalive', function(e) {
      UpdateValues(JSON.parse(e.data));
    }, false);

    source.addEventListener('update', function(e) {
      UpdateValues(JSON.parse(e.data));
    }, false);
  }
}

initES();

function DrawConfig(cfg) {
  UpdateValues(cfg)
}

function GetCfg() {
  ParseJsonQ("/config/get", DrawConfig);
}

GetCfg()
