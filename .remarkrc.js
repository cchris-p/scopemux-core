const stringify = require("remark-stringify");

module.exports = {
  settings: {
    bullet: "-",      // Force dashes for bullets
    emphasis: "_",    // Use underscore for italics
    strong: "*",      // Use asterisk for bold
    rule: "-",        // Use dashes for horizontal rules
    listItemIndent: "one", // Optional: more compact bullets
  },
  plugins: [
    [
      stringify,
      {
        bullet: "-",
        rule: "-",
        strong: "*",
        emphasis: "_",
        listItemIndent: "one",
        handlers: {
          text: (node, _, context) => {
            return context.encode(node.value, {
              // Disable escaping of all characters, including "_"
              unsafe: []
            });
          }
        }
      }
    ]
  ]
};
