if(typeof Duktape !== 'undefined') {
    Object.defineProperty(new Function('return this')(), 'window', { value: new Function('return this')(), writable: false, enumerable: true, configurable: false});
    window.JSON = {}
    window.JSON.stringify = function (obj) {
        return Duktape.enc('jc', obj);
    }
    window.JSON.parse = function (str) {
        return Duktape.dec('jc', str);
    }
}

M = {
    compare: function(val, targ) {
        return JSON.stringify(targ) === JSON.stringify(val);
    },
    copy: function(x) {
        return JSON.parse(JSON.stringify(x));
    }
}

___changes = {}
___reg = {};

function get_var(name) {
    if(name.search('.') === -1) {
        return window[name];
    }
    else {
        var v = window;
        var splitted = name.split('.');
        for(var _k in splitted) {
            var k = splitted[_k];
            if(!(k in v)) {
                v[k] = {};
            }
            v = v[k];
        }
        return v;
    }
}

function set_var(name, val) {
    var v = window;
    var splitted = name.split('.');
    var _k;
    for(_k in splitted) {
        var k = splitted[_k];
        if(!(k in v) || _k !== splitted.length - 1)
            v[k] = {};
        else if(_k !== splitted.length -1)
            v = v[k];
    }
    v[k] = val;
}
function add(name, init) {
    set_var(name, init);
    window.___reg[name] = {
        old: M.copy(init),
        var: name
    };
}

function poll_changes() {
    for(var a in window.___reg) {
        var v = window.___reg[a];
        var new_value = get_var(v.var);
        if(!(M.compare(v.old, new_value))) {
            ___changes[v.var] = new_value;
        }
    }
}

function get_changes() {
    var changes = {};
    for(var k in ___changes) {
        changes[k] = JSON.stringify(___changes[k]);
    }
    return JSON.stringify(changes);
}

function apply_changes() {
    for(var v in window.___changes) {
        window.___reg[v].old = M.copy(get_var(v));
    }
}

function restore_changes() {
    for (var v in window.___changes) {
        set_var(v, M.copy(window.___reg[v].old));
    }
    clear_changes();
}

function clear_changes(scope) {
    window.___changes = {}
}

function flush() {
    apply_changes();
    clear_changes();
}

function log_window() {
    var Z = {};
    for(var k in window) {
        if(k !== "window" && k !== "Z" && typeof window[k] !== "function" && k !== "ROS") {
            Z[k] = window[k];
        }
    }
    return JSON.stringify(Z);
}

// add("a", {"b":1})
// add("b", {"a":1})
// add("b.b", {"x":2})
// b.b.x = 3
// poll_changes()
// console.log(get_changes())
// flush()
// b.b.z = 2

// poll_changes()
// console.log(get_changes())
// flush()

// add("b.b.z", 4)
// console.log(b.b.z)
// poll_changes()
// console.log(get_changes())
// flush()
