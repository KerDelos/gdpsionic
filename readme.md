# Setup

## Godot Setup
* Clone the Godot repository. https://docs.godotengine.org/en/stable/development/compiling/getting_source.html
* Switch to branch 3.2. In the command line you may enter `git checkout 3.2`.
* Make sure you can build the Godot repository by following the instructions here depending on you platform : https://docs.godotengine.org/en/stable/development/compiling/index.html#

## Gdpsionic setup
* In the godot/modules folder clone the Psionic-Godot repository with the following command : git clone --recurse-submodules https://github.com/KerDelos/gdpsionic.git
* If you are on **windows** (no need to do anything like that if you're on **linux**) :
    * Open the SCsub file at the root of this repo
    * Look for a line saying with somethin like `# for Windows comment everything above and uncomment below`
    * Comment everything above this line and uncomment everything below.  \`\`\` This is how you comment in an SCsub file \`\`\`
    * Sorry the build system isn't yet automatically multiplaform.
* Recompile Godot just like you did during the Godot setup.
* Enjoy !

## Getting Started
Sorry I haven't taken the time to write a proper tutorial for now but you can look at how the template project works https://github.com/KerDelos/template_godot_psionic_project to get started.



