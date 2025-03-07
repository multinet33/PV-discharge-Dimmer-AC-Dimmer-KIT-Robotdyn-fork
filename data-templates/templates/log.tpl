<!doctype html>
<html lang="fr" class="">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no" />
    <title>Console logs</title>
    <%text>
    <script>
      var x = null,
        lt,
        to,
        tp,
        pc = "";
      eb = (s) => document.getElementById(s);
      qs = (s) => document.querySelector(s);
      sp = (i) => (eb(i).type = eb(i).type === "text" ? "password" : "text");
      wl = (f) => window.addEventListener("load", f);
      var sn = 0,
        id = 0,
        ft,
        ltm = 2345;
      function l(p) {
        var c,
          o = "";
        clearTimeout(lt);
        clearTimeout(ft);
        t = eb("t1");
        if (p == 1) {
          c = eb("c1");
          o = "&c1=" + encodeURIComponent(c.value);
          c.value = "";
          t.scrollTop = 1e8;
          sn = t.scrollTop;
        }
        if (t.scrollTop >= sn) {
          if (x != null) {
            x.abort();
          }
          x = new XMLHttpRequest();
          x.onreadystatechange = () => {
            if (x.readyState == 4 && x.status == 200) {
              var z, d;
              d = x.responseText.split(/}1/);
              id = d.shift();
              if (d.shift() == 0) {
                t.value = "";
              }
              z = d.shift();
              if (z.length > 0) {
                t.value += z;
              }
              t.scrollTop = 1e8;
              sn = t.scrollTop;
              clearTimeout(ft);
              lt = setTimeout(l, ltm);
            }
          };
          x.open("GET", "cs?c2=" + id + o, true);
          x.send();
          ft = setTimeout(l, 2e4);
        } else {
          lt = setTimeout(l, ltm);
        }
        return false;
      }
      wl(l);
      var hc = [],
        cn = 0;
      function h() {
        eb("c1").addEventListener("keydown", (e) => {
          var b = eb("c1"),
            c = e.keyCode;
          if (38 == c || 40 == c) {
            b.autocomplete = "off";
            setTimeout(
              (b) => {
                b.focus();
                b.setSelectionRange(1e9, 1e9);
              },
              0,
              b,
            );
          }
          38 == c
            ? (++cn > hc.length && (cn = hc.length), (b.value = hc[cn - 1] || ""))
            : 40 == c
              ? (0 > --cn && (cn = 0), (b.value = hc[cn - 1] || ""))
              : 13 == c && (hc.length > 19 && hc.pop(), hc.unshift(b.value), (cn = 0));
        });
      }
      wl(h);
      function jd() {
        var t = 0,
          i = document.querySelectorAll("input,button,textarea,select");
        while (i.length >= t) {
          if (i[t]) {
            i[t]["name"] =
              i[t].hasAttribute("id") && !i[t].hasAttribute("name") ? i[t]["id"] : i[t]["name"];
          }
          t++;
        }
      }
      function sf(s) {
        var t = 0,
          i = document.querySelectorAll(".hf");
        while (i.length >= t) {
          if (i[t]) {
            i[t].style.display = s ? "block" : "none";
          }
          t++;
        }
      }
      wl(jd);
    </script>
    <style>
      div,
      fieldset,
      input,
      select {
        padding: 5px;
        font-size: 1em;
      }
      fieldset {
        background: #4f4f4f;
      }
      p {
        margin: 0.5em 0;
      }
      input {
        width: 100%;
        box-sizing: border-box;
        -webkit-box-sizing: border-box;
        -moz-box-sizing: border-box;
        background: #dddddd;
        color: #000000;
      }
      input[type="checkbox"],
      input[type="radio"] {
        width: 1em;
        margin-right: 6px;
        vertical-align: -1px;
      }
      input[type="range"] {
        width: 99%;
      }
      select {
        width: 100%;
        background: #dddddd;
        color: #000000;
      }
      textarea {
        resize: vertical;
        width: 98%;
        height: 318px;
        padding: 5px;
        overflow: auto;
        background: #1f1f1f;
        color: #65c115;
      }
      body {
        text-align: center;
        font-family: verdana, sans-serif;
        background: #252525;
      }
      td {
        padding: 0px;
      }
      button {
        border: 0;
        border-radius: 0.3rem;
        background: #1fa3ec;
        color: #faffff;
        line-height: 2.4rem;
        font-size: 1.2rem;
        width: 100%;
        -webkit-transition-duration: 0.4s;
        transition-duration: 0.4s;
        cursor: pointer;
      }
      button:hover {
        background: #0e70a4;
      }
      .bred {
        background: #d43535;
      }
      .bred:hover {
        background: #931f1f;
      }
      .bgrn {
        background: #47c266;
      }
      .bgrn:hover {
        background: #5aaf6f;
      }
      a {
        color: #1fa3ec;
        text-decoration: none;
      }
      .p {
        float: left;
        text-align: left;
      }
      .q {
        float: right;
        text-align: right;
      }
      .r {
        border-radius: 0.3em;
        padding: 2px;
        margin: 6px 2px;
      }
      .hf {
        display: none;
      }
    </style>
    </%text>
  </head>
  <body>
    <div style="text-align: left; display: inline-block; color: #eaeaea; min-width: 340px">
      <div style="text-align: center; color: #eaeaea">
        <noscript>
          Pour utiliser Le logger, veuillez activer JavaScript
          <br />
        </noscript>
        <h3>DATA LOGGER</h3>
      </div>
      <br />
      <textarea readonly id="t1" cols="340" wrap="off"></textarea>
      <br />
      <br />
      <div id="but2d" style="display: block"></div>
      <p></p>
      <div style="text-align: right; font-size: 11px">
        <hr />
      </div>
    </div>
  </body>
</html>
