"use strict"
let current_page = "";

let username = "__linggo_guest__";
let passwd = "__linggo_guest__";
let userinfo = null;

let quiz_data_list = [];
let quiz_word = "";
let quiz_word_index = 0;
let quiz_prompt_data = null;
let quiz_prompted = false;
let quiz_prompt_panel_isopen = [false, false, false, false];

let memorize_word = "";
let memorize_index = 0;
let memorize_meaning = "";

let search_data = null;

let mutationObserver = null;
let status_updater = null;
function register(u_name, pwd) {
    $.ajax({
        type: 'GET',
        url: "api/register",
        data:
            {
                username: u_name,
                passwd: pwd,
            },
        success: function (result) {
            if (result["status"] === "success") {
                username = u_name;
                passwd = pwd;
                userinfo = result;
                update_account_info();
                mdui.snackbar("注册成功");
                document.getElementById('register-dialog-close').click();
            } else
                mdui.snackbar(result["message"]);
        },
    });
}

function login(u_name, pwd) {
    $.ajax({
        type: 'GET',
        url: "api/login",
        data:
            {
                username: u_name,
                passwd: pwd
            },
        success: function (result) {
            if (result["status"] === "success") {
                username = u_name;
                passwd = pwd;
                userinfo = result;
                update_account_info();
                mdui.snackbar("欢迎回来, " + u_name);
                document.getElementById('login-dialog-close').click();
            } else
                mdui.snackbar(result["message"]);
        },
    });
}

function update_account_info() {
    $("#account-name").html(username);
    $("#account-info").removeAttr("mdui-dialog");
    window.localStorage.setItem("user", JSON.stringify({username: username, passwd: passwd}));
    window.localStorage.setItem("userinfo", JSON.stringify(userinfo));
}
function init_page(with_appbar) {
    with_appbar = typeof with_appbar !== 'undefined' ? with_appbar : true;
    if (with_appbar) {
        $.ajax({
            type: 'GET',
            url: "fragments/appbar.html",
            async: false,
            success: function (result) {
                $("#appbar-placeholder").html(result);
                $("#page-" + current_page).addClass("mdui-list-item-active");
                $("#page-" + current_page + " > div").removeClass("mdui-text-color-black-text");
                mdui.mutation();
            }
        });
        var search_form = document.getElementById("search-form");
        search_form.onsubmit = function (event) {
            var search_word = document.getElementById("search-value").value;
            $.ajax({
                type: 'POST',
                url: "api/search",
                data:
                    {
                        word: search_word,
                        username: username,
                        passwd: passwd
                    },
                success: function (result) {
                    if (result["status"] === "success") {
                        window.sessionStorage.setItem("search_result", JSON.stringify(result));
                        window.location.href = "search.html";
                    } else
                        mdui.snackbar(result["message"]);
                },
            });
            event.preventDefault();
        }

        var login_form = document.getElementById("login-form");
        login_form.onsubmit = function (event) {
            var u_name = document.getElementById("login-user-name").value;
            var pwd = document.getElementById("login-password").value;
            login(u_name, pwd);
            event.preventDefault();
        }

        var register_form = document.getElementById("register-form");
        register_form.onsubmit = function (event) {
            var u_name = document.getElementById("register-user-name").value;
            var pwd = document.getElementById("register-password").value;
            register(u_name, pwd);
            event.preventDefault();
        }

        mutationObserver = new MutationObserver(function (mutations) {
            mutations.forEach(function (mutation) {
                if ($("#toolbar-search").hasClass('mdui-textfield-expanded')) {
                    $(".toolbar-hidden-when-search").addClass("search-bar-hidden");
                    $("#toolbar-search-container").addClass("search-bar-container");
                } else {
                    $(".toolbar-hidden-when-search").removeClass("search-bar-hidden");
                    $("#toolbar-search-container").removeClass("search-bar-container");
                }
            });
        });
        mutationObserver.observe($('#toolbar-search')[0], {
            attributes: true,
        });
    }
    var user = JSON.parse(window.localStorage.getItem("user"));
    if (!$.isEmptyObject(user) && user["username"] !== "__linggo_guest__") {
        username = user["username"];
        passwd = user["passwd"];
    }
}

function init_content(page) {
    switch (page) {
        case "marked":
            $.ajax({
                type: 'GET',
                url: "api/get_marked",
                data:
                    {
                        username: username,
                        passwd: passwd
                    },
                success: function (result) {
                    if (result["status"] === "success") {
                        var content = '<div class="mdui-panel" mdui-panel>';
                        for (let i = result["marked_words"].length - 1; i >= 0; i--) {
                            content += marked_explanation_panel(result["marked_words"][i]);
                        }
                        content += '</div>'
                        $("#marked-words").html(content);
                        mdui.mutation();
                        $("#loading").remove();
                        $("#marked-data").removeClass("mdui-hidden");
                    } else {
                        mdui.snackbar(result["message"]);
                    }
                }
            });
            break;
        case "memorize":
            next_word(false);
            var Element = document.getElementById('memorize-swipe');
            var mc = new Hammer(Element);
            mc.on("swiperight", function (ev) {
                prev_word();
                $(".mdui-bottom-nav-fixed").append(
                    '<button class="mdui-fab mdui-ripple swiperight">' +
                    '<i class="mdui-icon material-icons">navigate_before</i></button>')
                $(".swiperight").fadeTo('normal', 0.01,
                    function () {
                        $(this).slideUp('normal', function () {
                            $(this).remove();
                        });
                    });
                ;
            });
            mc.on("swipeleft", function (ev) {
                next_word(true);
                $(".mdui-bottom-nav-fixed").append(
                    '<button class="mdui-fab mdui-ripple swipeleft">' +
                    '<i class="mdui-icon material-icons">navigate_next</i></button>')
                $(".swipeleft").fadeTo('normal', 0.01,
                    function () {
                        $(this).slideUp('normal', function () {
                            $(this).remove();
                        });
                    });
                ;
            });
            break;
        case "quiz":
            next_quiz(-1);
            break;
        case "search":
            if (search_data == null)
                search_data = JSON.parse(window.sessionStorage.getItem("search_result"));
            else
                window.sessionStorage.setItem("search_result", search_data);
            var content = '<div class="mdui-panel" mdui-panel>';
            for (var word in search_data["words"])
                content += search_explanation_panel(search_data["words"], word);
            content += '</div>'
            $("#search-result").html(content);
            $("#search-title").html(search_data["message"]);
            mdui.mutation();
            break;
        case "about":
            $.ajax({
                type: 'GET',
                url: "api/version",
                success: function (result) {
                    $("#version").html(result["version"]);
                }
            });
            break;
    }
}

function load_body(page) {
    if (status_updater != null)
        window.clearInterval(status_updater);
    window.sessionStorage.setItem("page", page);
    if (current_page !== "") {
        $("#page-" + current_page).removeClass("mdui-list-item-active");
        $("#page-" + current_page + " > div").addClass("mdui-text-color-black-text");
    }
    current_page = page;
    $.ajax({
        type: 'GET',
        url: "fragments/" + page + ".html",
        async: false,
        success: function (result) {
            $("#page-" + page).addClass("mdui-list-item-active");
            $("#page-" + page + " > div").removeClass("mdui-text-color-black-text");
            let doc = new DOMParser().parseFromString(result, 'text/html');
            let bodyClassList = doc.querySelector('body').classList;
            let content = doc.querySelector('#content');
            document.getElementById('content').replaceWith(content);
            document.getElementsByTagName('body')[0].classList = bodyClassList;
            init_content(page);
            if (window.innerWidth < 599)
                document.getElementById('drawer-button')?.click();
        }
    });
}

function init_prompt() {
    $("#explanation").html('<div class="mdui-panel" mdui-panel>' +
        prompt_explanation_panel(quiz_prompt_data, "A", quiz_prompt_panel_isopen[0]) +
        prompt_explanation_panel(quiz_prompt_data, "B", quiz_prompt_panel_isopen[1]) +
        prompt_explanation_panel(quiz_prompt_data, "C", quiz_prompt_panel_isopen[2]) +
        prompt_explanation_panel(quiz_prompt_data, "D", quiz_prompt_panel_isopen[3]) +
        '</div>');
    mdui.mutation();
}

function init_explanation(word_index) {
    $.ajax({
        type: 'GET',
        url: "api/get_explanation",
        data:
            {
                word_index: word_index
            },
        success: function (result) {

            if (result["status"] === "success") {
                $("#explanation-" + word_index).html(result["explanation"]);
            } else {
                mdui.snackbar(result["message"])
            }
        }
    });
}

function save_quiz_prompt_panel_status() {
    var panels = document.getElementsByClassName("mdui-panel-item");
    quiz_prompt_panel_isopen[0] = panels[0].classList.length > 1;
    quiz_prompt_panel_isopen[1] = panels[1].classList.length > 1;
    quiz_prompt_panel_isopen[2] = panels[2].classList.length > 1;
    quiz_prompt_panel_isopen[3] = panels[3].classList.length > 1;
}

function update_memorize_data(result) {
    memorize_word = result["word"]["word"];
    speak(memorize_word)
    memorize_index = result["word"]["word_index"];
    memorize_meaning = result["word"]["meaning"];
    $("#explanation").html(result["content"]);
    $("#search-locate-value").val("");
    $('#locate-value').attr('value', memorize_index)
    mdui.updateSliders();
}

function set_memorize_word(word_index) {
    $.ajax({
        type: 'GET',
        url: "api/set_memorize_word",
        data:
            {
                word_index: word_index,
                username: username,
                passwd: passwd
            },
        success: function (result) {

            if (result["status"] === "success")
                update_memorize_data(result);
            else
                mdui.snackbar(result["message"]);
        }
    });
}

function prev_word() {
    $.ajax({
        type: 'GET',
        url: "api/prev_memorize_word",
        data: {
            username: username,
            passwd: passwd
        },
        success: function (result) {

            if (result["status"] === "success")
                update_memorize_data(result);
            else
                mdui.snackbar(result["message"]);
        }
    });
}
function next_word(next) {
    next = typeof next !== 'undefined' ? next : false;
    $.ajax({
        type: 'GET',
        url: "api/memorize_word",
        data:
            {
                next: next,
                username: username,
                passwd: passwd
            },
        success: function (result) {

            if (result["status"] === "success")
                update_memorize_data(result);
            else
                mdui.snackbar(result["message"]);
        }
    });
}

function mark_word(word_index) {
    $.ajax({
        type: 'GET',
        url: "api/mark_word",
        data:
            {
                word_index: word_index,
                username: username,
                passwd: passwd
            },
        success: function (result) {

            if (result["status"] === "success")
                mdui.snackbar("收藏成功");
            else
                mdui.snackbar(result["message"]);
        }
    });
}

function unmark_word(word_index) {
    $.ajax({
        type: 'GET',
        data:
            {
                word_index: word_index,
                username: username,
                passwd: passwd
            },
        url: "api/unmark_word",
        success: function (result) {

            if (result["status"] === "success")
                mdui.snackbar("取消收藏成功");
            else
                mdui.snackbar(result["message"]);
        }
    });
}

function prev_quiz() {
    if (quiz_data_list.length === 1) {
        mdui.snackbar("没有上一个了");
    } else {
        quiz_data_list.pop();
        quiz_word = quiz_data_list[quiz_data_list.length - 1]["quiz_word"];
        quiz_word_index = quiz_data_list[quiz_data_list.length - 1]["word_index"];
        apply_quiz(quiz_data_list[quiz_data_list.length - 1]["quiz"]);
    }
}

function apply_quiz(new_quiz) {
    $("#A").parent().removeClass("mdui-color-red");
    $("#A").parent().removeClass("mdui-color-light-green");
    $("#B").parent().removeClass("mdui-color-red");
    $("#B").parent().removeClass("mdui-color-light-green");
    $("#C").parent().removeClass("mdui-color-red");
    $("#C").parent().removeClass("mdui-color-light-green");
    $("#D").parent().removeClass("mdui-color-red");
    $("#D").parent().removeClass("mdui-color-light-green");
    $("#explanation").html("");
    $("#question").html(new_quiz["question"]);
    $("#A").html("A. " + new_quiz["options"]["A"]);
    $("#B").html("B. " + new_quiz["options"]["B"]);
    $("#C").html("C. " + new_quiz["options"]["C"]);
    $("#D").html("D. " + new_quiz["options"]["D"]);
    quiz_prompted = false;
    quiz_prompt_panel_isopen = [false, false, false, false];
}

function next_quiz(word_index) {
    word_index = typeof word_index !== 'undefined' ? word_index : -1;
    $.ajax({
        type: 'GET',
        url: "api/get_quiz",
        data:
            {
                word_index: word_index,
                username: username,
                passwd: passwd
            },
        success: function (result) {
            quiz_data_list.push(result)
            quiz_word = result["word"]["word"]
            quiz_word_index = result["word"]["word_index"]
            apply_quiz(quiz_data_list[quiz_data_list.length - 1]["quiz"])
        }
    });
}

function quiz_select(opt) {
    if (opt === quiz_data_list[quiz_data_list.length - 1]["quiz"]["answer"]) {
        if (!quiz_prompted) {
            $.ajax({
                type: 'GET',
                url: "api/quiz_passed",
                data:
                    {
                        word_index: quiz_word_index,
                        username: username,
                        passwd: passwd
                    },
            });
        }
        next_quiz()
        apply_quiz(quiz_data_list[quiz_data_list.length - 1]["quiz"])
    } else {
        if (!quiz_prompted) {
            $.ajax({
                type: 'GET',
                url: "api/quiz_failed",
                data:
                    {
                        word_index: quiz_word_index,
                        username: username,
                        passwd: passwd
                    },
            });
        }
        $("#" + opt).parent().addClass("mdui-color-red");
    }
}

function get_explanation_panel(title, summary, body, actions, open, open_actions) {
    actions = typeof actions !== 'undefined' ? actions : "";
    open_actions = typeof open_actions !== 'undefined' ? open_actions : "";
    open = typeof open !== 'undefined' ? open : true;
    let ret = '';
    if (open)
        ret += '<div class="mdui-panel-item mdui-panel-item-open">';
    else
        ret += '<div class="mdui-panel-item" onclick="' + open_actions + '">';
    ret += '<div class="mdui-panel-item-header">' +
        '<div class="mdui-panel-item-title">' + title + '</div>' +
        '<div class="mdui-panel-item-summary">' + summary + '</div>' +
        '<i class="mdui-panel-item-arrow mdui-icon material-icons">keyboard_arrow_down</i>' +
        '</div><div class="mdui-panel-item-body">' + body;
    if (actions !== "") {
        ret += '<div class="mdui-float-right">' + actions + '</div>';
    }
    ret += '</div></div>';
    return ret;
}

function prompt_explanation_panel(result, opt, open) {
    let actions = "";
    if (result[opt]["is_marked"]) {
        actions = '' +
            '<button class=\"mdui-btn mdui-ripple\" onclick=\"save_quiz_prompt_panel_status();unmark_word('
            + quiz_data_list[quiz_data_list.length - 1]["quiz"]["indexes"][opt] + ');' +
            'quiz_prompt_data[\'' + opt + '\'][\'is_marked\']=false;init_prompt()\"><i class="mdui-icon material-icons">delete</i>取消收藏</button>';
    } else {
        actions = '<button class=\"mdui-btn mdui-ripple\" onclick=\"save_quiz_prompt_panel_status();mark_word('
            + quiz_data_list[quiz_data_list.length - 1]["quiz"]["indexes"][opt] + ');' +
            'quiz_prompt_data[\'' + opt + '\'][\'is_marked\']=true;init_prompt()\"><i class="mdui-icon material-icons">star</i>收藏</button>';
    }
    return get_explanation_panel(opt, quiz_data_list[quiz_data_list.length - 1]["quiz"]["options"][opt],
        result[opt]["explanation"], actions, open);
}

function search_explanation_panel(words, pos) {
    let actions = "";
    if (words[pos]["is_marked"]) {
        actions = '<button class=\"mdui-btn mdui-ripple\" onclick=\"unmark_word('
            + words[pos]["word"]["word_index"] + ');' +
            'search_data[\'words\'][' + pos + '][\'is_marked\']=false;' +
            'init_search_result()\"><i class="mdui-icon material-icons">delete</i>取消收藏</button>';
    } else {
        actions = '<button class=\"mdui-btn mdui-ripple\" onclick=\"mark_word('
            + words[pos]["word"]["word_index"] + ');' +
            'search_data[\'words\'][' + pos + '][\'is_marked\']=true;' +
            'init_search_result()\"><i class="mdui-icon material-icons">star</i>收藏</button>';

    }
    return get_explanation_panel(words[pos]["word"]["word"],
        words[pos]["word"]["meaning"],
        '<span id="explanation-' + words[pos]["word"]["word_index"] + '">' +
        '<div class="mdui-progress"><div class="mdui-progress-indeterminate"></div></div></span>',
        actions, false, 'init_explanation(' + words[pos]["word"]["word_index"] + ')');
}

function marked_explanation_panel(word) {
    return get_explanation_panel(word["word"]["word"],
        word["word"]["meaning"],
        '<span id="explanation-' + word["word"]["word_index"] + '">' +
        '<div class="mdui-progress"><div class="mdui-progress-indeterminate"></div></div></span>',
        '<button class=\"mdui-btn mdui-ripple\" onclick=\"unmark_word('
        + word["word_index"] + ');$(this).parent().parent().parent().fadeTo(\'normal\', 0.01, function(){$(this).slideUp(\'normal\', function() {$(this).remove();});});;\">' +
        '<i class="mdui-icon material-icons">delete</i>取消收藏</button>',
        false, 'init_explanation(' + word["word"]["word_index"] + ')');
}

function quiz_prompt(opt) {
    if (!quiz_prompted) {
        quiz_prompted = true;
        $("#" + opt).parent().addClass("mdui-color-red");
        $.ajax({
            type: 'GET',
            url: "api/quiz_prompt",
            data:
                {
                    A_index: quiz_data_list[quiz_data_list.length - 1]["quiz"]["indexes"]["A"],
                    B_index: quiz_data_list[quiz_data_list.length - 1]["quiz"]["indexes"]["B"],
                    C_index: quiz_data_list[quiz_data_list.length - 1]["quiz"]["indexes"]["C"],
                    D_index: quiz_data_list[quiz_data_list.length - 1]["quiz"]["indexes"]["D"],
                    word_index: quiz_word_index,
                    username: username,
                    passwd: passwd
                },
            success: function (result) {

                quiz_prompt_data = result;
                if (result["status"] === "success") {
                    init_prompt()
                } else {
                    mdui.snackbar(result["message"])
                }
            },
        });
        $("#" + quiz_data_list[quiz_data_list.length - 1]["quiz"]["answer"]).parent().addClass("mdui-color-light-green");
    }
}

function speak(word) {
    var url = "http://dict.youdao.com/dictvoice?type=0&audio=" + encodeURI(word.replaceAll(' ', '-'));
    var n = new Audio(url);
    n.play();
}

function clear_marks() {
    $.ajax({
        type: 'GET',
        url: "api/clear_marks",
        data: {
            username: username,
            passwd: passwd
        },
        success: function (result) {
            quiz_prompt_data = result;
            if (result["status"] === "success") {
                init_content("marked");
                mdui.snackbar("已清除所有收藏");
            } else {
                mdui.snackbar(result["message"]);
            }
        }
    });
}