# Step 1
with open("main1.bin", "rb") as f1:
    data1 = f1.read()

# Step 2
with open("main2.bin", "rb") as f2:
    data2 = f2.read()

# Step 3
combined = data1 + data2

# Step 4
with open("combined.bin", "wb") as out:
    out.write(combined)