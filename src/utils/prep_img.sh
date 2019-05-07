var=0
for jpeg in Dress-master/*/*.jpg
do
    var=$((var+1))
    mv $jpeg "dress-jpeg/$var.jpg"
done
