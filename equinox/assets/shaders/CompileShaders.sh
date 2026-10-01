mkdir -p ./bin

if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    GLSLC="$VULKAN_SDK/Bin/glslc.exe"
else
    GLSLC="glslc"
fi

for file in *.vert *.frag; do
    echo "Compiling $file..."
    $GLSLC $file -o ./bin/$file.spv
done

echo ""
read -p "Finished. Please check if there are any compilation errors and press Enter to close this window."
