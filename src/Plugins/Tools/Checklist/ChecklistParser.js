/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
.pragma library

/*
    Markdown procedure parser. Nothing else is recognised:
      # Title           procedure title, the text below it is the description (intro)
      ## Step title     a step, the text below it is shown to the operator
      ```js ... ```     in a step: the step script

    All scripts share one global scope, so values used by several steps are declared
    in the script of the first step that needs them; there is no shared block.

    parse(text) -> { title, intro, steps: [{ title, body, code }] }
*/
function parse(text)
{
    var doc = { "title": "", "intro": "", "steps": [] }
    var lines = (text ? text : "").split(/\r?\n/)
    var step = null      // current step, null while in the description
    var body = []        // markdown lines of the current section
    var code = null      // lines of the open ```js block

    function flush()
    {
        var md = body.join("\n").trim()
        if(step) step.body = md
        else doc.intro = md
        body = []
    }

    for (var i = 0; i < lines.length; ++i) {
        var line = lines[i]
        if(code !== null){
            if(/^```/.test(line)){
                step.code += code.join("\n") + "\n"
                code = null
            }else code.push(line)
            continue
        }
        if(step && /^```\s*js\b/.test(line)){
            code = []
            continue
        }
        if(/^#\s+/.test(line)){
            doc.title = line.replace(/^#\s+/, "").trim()
            continue
        }
        if(/^##\s+/.test(line)){
            flush()
            step = { "title": line.replace(/^##\s+/, "").trim(), "body": "", "code": "" }
            doc.steps.push(step)
            continue
        }
        body.push(line)
    }
    flush()
    return doc
}
