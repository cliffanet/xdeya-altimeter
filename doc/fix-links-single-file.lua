function Link (link)
  link.target = link.target:gsub('.+%.md%#(.+)', '#%1')
  return link
end
