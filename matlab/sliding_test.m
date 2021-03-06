%% 1. Offline
clear;
close all;
clc;

%% Set parameters at here!!
bagfile_path = '../worlds/occupancy_inflated_seq.bag'; 
nav_msg_topic_name = '/costmap_node/costmap/costmap';
car_width = 2;
map_resolution = 0.5;
grid_resolution = 0.5;
threshold = 0.5;
max_steering_angle = pi/20;
max_smoothing_iteration = 0;
smoothing_margin = 0.5;
nominal_speed = 1;
corridor_dt = 1;
corridor_horizon = 10;

% current_state.position = [0 + 25 0 + 15];
% current_state.velocity = 0;
% current_state.theta = pi/2;
% goal_state.position = [50 + 25 70 + 15];

lanePath = [1 0 ; 
            1 71; 
            50 71;]; % lanePath = [lanePoint; lanePoint; lanePoint; ...]
laneWidth = [8;
                   12];

%% Lane path preprocessing
laneAngle = zeros([size(lanePath, 1) - 1, 1]);
for i = 1:size(lanePath)-1
    direction = lanePath(i+1,:) - lanePath(i,:);
    laneAngle(i) = atan2(direction(2), direction(1));
end
% get laneGridPoint before transformation
laneGridPoint_ = [];
laneGridAngle = [];
laneGridWidth = [];
for i = 1:size(lanePath)-1
    lanePath_length = norm(lanePath(i+1,:) - lanePath(i,:));
    current_length = 0;
    while current_length < lanePath_length
        alpha = current_length / lanePath_length;
        laneGridPoint_ = [laneGridPoint_; lanePath(i+1,:) * alpha + lanePath(i,:) * (1-alpha)];
        laneGridAngle = [laneGridAngle; laneAngle(i)];
        laneGridWidth = [laneGridWidth; laneWidth(i)];
        current_length = current_length + grid_resolution;
    end
end

%% 2. Realtime Works
%% Read map
bag = rosbag(bagfile_path);
bSel = select(bag,'Topic',nav_msg_topic_name);
msgStructs = readMessages(bSel,'DataFormat','struct');

figure(1)
set(gcf,'Position',[100 100 1000 1000])
for i_msg = 1 : size(msgStructs,1)
msgMap = msgStructs{i_msg};
msg = rosmessage('nav_msgs/OccupancyGrid');
msg.Info.Height = msgMap.Info.Height;
msg.Info.Width = msgMap.Info.Width;
msg.Info.Resolution = msgMap.Info.Resolution;
msg.Data = msgMap.Data;
map = readBinaryOccupancyGrid(msg);

% %% Find closest point to lanePath
% [p_lane_start, lane_index_start] = getCloestPointToLanePath(current_state.position, lanePath, 1);
% [p_lane_goal, lane_index_goal] = getCloestPointToLanePath(goal_state.position, lanePath, 1);


%% update initialLane
translation = [msgMap.Info.Origin.Position.X msgMap.Info.Origin.Position.Y];
laneGridPoint = laneGridPoint_ - translation;


%% update start_time
if i_msg == 1
    start_time = double(msgMap.Header.Stamp.Sec) + double(msgMap.Header.Stamp.Nsec) / 1000000000;
    current_time = 0;
else
    current_time = double(msgMap.Header.Stamp.Sec) + double(msgMap.Header.Stamp.Nsec) / 1000000000 - start_time;
end


%% Grid generation
%for laneGridPoint check side point and get middle points
i_tree = 1;
laneTree = [];
% leftGrid = [];
% rightGrid = [];


for i = 1:size(laneGridPoint)
    col_size = 2 * (2 * floor(laneGridWidth(i)/2/grid_resolution) + 1);
    side_point = zeros(col_size/2, 2);
    
    left_angle = laneGridAngle(i) + pi/2;
    right_angle = laneGridAngle(i) - pi/2; 
        
    side_point(floor(laneGridWidth(i)/2/grid_resolution)+1,:) = laneGridPoint(i,:);
    for j = 1:floor(laneGridWidth(i)/2/grid_resolution)
        left_point = laneGridPoint(i,:) + j * grid_resolution * [cos(left_angle) sin(left_angle)];
        right_point = laneGridPoint(i,:) + j * grid_resolution * [cos(right_angle) sin(right_angle)];
        
        side_point(floor(laneGridWidth(i)/2/grid_resolution)+1+j,:) = left_point;
        side_point(floor(laneGridWidth(i)/2/grid_resolution)+1-j,:) = right_point;
    end
    
    start_idx = 0;
    for k = 1:size(side_point,1)
        if checkOccupancy(map,side_point(k,:)) < threshold
            if start_idx == 0
                start_idx = k;
            end
            if k == size(side_point,1)
                midPoint = (side_point(k,:) + side_point(start_idx,:))/2;
                parents = findParents(map, laneTree, i, midPoint);
                laneTree(i_tree,:).id = i;
                laneTree(i_tree,:).leftPoint = side_point(k,:);
                laneTree(i_tree,:).midPoint = midPoint;
                laneTree(i_tree,:).rightPoint = side_point(start_idx,:);
                laneTree(i_tree,:).parents = parents;
                i_tree = i_tree + 1;
            end
        elseif start_idx > 0
            midPoint = (side_point(k-1,:) + side_point(start_idx,:))/2;
            parents = findParents(map, laneTree, i, midPoint);
            laneTree(i_tree,:).id = i;
            laneTree(i_tree,:).leftPoint = side_point(k-1,:);
            laneTree(i_tree,:).midPoint = midPoint;
            laneTree(i_tree,:).rightPoint = side_point(start_idx,:);
            laneTree(i_tree,:).parents = parents;
            i_tree = i_tree + 1;
            start_idx = 0;
        else
            start_idx = 0;
        end
    end
end

%% Find proper collision-free midPoint
i_tree = size(laneTree, 1);
tail = laneTreeDFS(laneTree, i_tree);
midPoints = [];

% if tail == -1
%     hold on
%     show(map)
%     
%     for i_tree = 1:size(laneTree, 1)    
%         midPoints(i_tree, :) = laneTree(i_tree).midPoint;
%         parents = laneTree(i_tree).parents;
%         if i_tree ~= 1
%             for i_parents = 1:size(parents,2)
%                 lineSegment = [midPoints(i_tree, :); laneTree(laneTree(i_tree).parents(i_parents)).midPoint];
%                 plot(lineSegment(:,1), lineSegment(:,2));
%             end
%         end
%     end
%     scatter(midPoints(:,1), midPoints(:,2), '.')
%     
%     
%     hold off
%     pause(0.01)
% end

midPoints = zeros(size(tail, 2), 2);
for i_tail = 1:size(tail, 2)
    midPoints(i_tail, :) = laneTree(tail(i_tail)).midPoint;
end

leftPoints = zeros(size(tail, 2), 2);
for i_tail = 1:size(tail, 2)
    leftPoints(i_tail, :) = laneTree(tail(i_tail)).leftPoint;
end

rightPoints = zeros(size(tail, 2), 2);
for i_tail = 1:size(tail, 2)
    rightPoints(i_tail, :) = laneTree(tail(i_tail)).rightPoint;
end


%% Find midGridAngle
midAngle = zeros(size(midPoints,1)-1, 1);
for i = 1:size(midAngle, 1)
    delta_angle = midPoints(i+1,:) - midPoints(i,:);
    angle = atan2(delta_angle(2), delta_angle(1));
    if(angle < 0)
        angle = angle + 2 * pi;
    end
    midAngle(i) = angle;
end

%% Smoothing using linear interpolation TODO: valid smoothing check
% i = 1;
% while i < size(midAngle, 1)
%     if(angDiff(midAngle(i+1), midAngle(i)) > max_steering_angle)
%         idx_start = i;
%         idx_end = min(i+3, size(midPoints,1));
%         delta = idx_end - idx_start;
%         for j = 1:delta-1
%             alpha = j/delta;
%             smoothingPoint = (1-alpha) * midPoints(idx_start,:) + alpha * midPoints(idx_end,:);
%             midPoints(idx_start+j,:) = smoothingPoint;
%         end
%         for j = 0:delta-1
%             delta_angle = midPoints(idx_start+j+1,:) - midPoints(idx_start+j,:);
%             angle = atan2(delta_angle(2), delta_angle(1));
%             if(angle < 0)
%                 angle = angle + 2 * pi;
%             end
%             midAngle(idx_start+j) = angle;
%         end
%         i = max(i-1 , 1);
%     else
%         i = i + 1;
%     end
% end

i = 1;
while i < size(midAngle, 1)
    if(angDiff(midAngle(i+1), midAngle(i)) > max_steering_angle)
        idx_start = i;
        idx_end = min(i+3, size(midPoints,1));
        delta = idx_end - idx_start;
        for j = 1:delta-1
            alpha = j/delta;
            smoothingPoint = (1-alpha) * midPoints(idx_start,:) + alpha * midPoints(idx_end,:);
            midPoints(idx_start+j,:) = smoothingPoint;
        end
        for j = 0:delta-1
            delta_angle = midPoints(idx_start+j+1,:) - midPoints(idx_start+j,:);
            angle = atan2(delta_angle(2), delta_angle(1));
            if(angle < 0)
                angle = angle + 2 * pi;
            end
            midAngle(idx_start+j) = angle;
        end
        i = max(i-1 , 1);
    else
        i = i + 1;
    end
end

% i = 1;
% while i < size(midAngle, 1)
%     for i_smooth = 0:max_smoothing_iteration-1
%         idx_start = max(i - i_smooth, 1);
%         idx_end = min(i+3+i_smooth, size(midPoints,1));
%         delta = idx_end - idx_start;
%         smoothingPoint = zeros(delta-1, 2);
%         is_smoothing_valid = true;
%         
%         for j = 1:delta-1
%             alpha = j/delta;
%             smoothingPoint(j,:) = (1-alpha) * midPoints(idx_start,:) + alpha * midPoints(idx_end,:);
%             leftMargin = leftPoints(idx_start+j,:) - smoothingPoint(j,:);
%             rightMargin = rightPoints(idx_start+j,:) - smoothingPoint(j,:);
%             if dot(leftMargin, rightMargin) >= 0 || norm(leftMargin) < smoothing_margin || norm(rightMargin) < smoothing_margin
%                 % collision occured!
%                 is_smoothing_valid = false;
%             end
%         end
%         
%         if is_smoothing_valid
%             for j = 1:delta-1
%                 midPoints(idx_start+j,:) = smoothingPoint(j,:);
%             end
%         else
%             break;
%         end
%     end
%     i = i + 1;  
% end

% window = 1;
% for i_smooth = 0:max_smoothing_iteration-1
%     smoothingPoints = zeros(size(midAngle, 1), 2);
%     for i = 1+window:size(midAngle, 1)-window
%         idx_start = i-window;
%         idx_end = i+window;
%         
%         delta = idx_end - idx_start + 1;
%         is_smoothing_valid = true;
%         
%         for j = idx_start:idx_end
%             smoothingPoints(i,:) = smoothingPoints(i,:) + midPoints(j,:) / delta;
%         end
%         
%         leftMargin = leftPoints(i,:) - smoothingPoints(i,:);
%         rightMargin = rightPoints(i,:) - smoothingPoints(i,:);
%         if dot(leftMargin, rightMargin) >= 0 || norm(leftMargin) < smoothing_margin || norm(rightMargin) < smoothing_margin
%             % collision occured!
%             smoothingPoints(i,:) = midPoints(i,:);
%         end
%     end
%     
%     midPoints(1+window:size(midAngle, 1)-window,:) = smoothingPoints(1+window:size(midAngle, 1)-window,:);
% end


% for i_smooth = 0:max_smoothing_iteration-1
%     for i  = 1:size(midAngle, 1)
% %         idx_start = max(i - i_smooth, 1);
% %         idx_end = min(i+3+i_smooth, size(midPoints,1));
%         idx_start = max(i, 1);
%         idx_end = min(i+3, size(midPoints,1));
%         
%         delta = idx_end - idx_s% i = 1;
% while i < size(midAngle, 1)
%     for i_smooth = 0:max_smoothing_iteration-1
%         idx_start = max(i - i_smooth, 1);
%         idx_end = min(i+3+i_smooth, size(midPoints,1));
%         delta = idx_end - idx_start;
%         smoothingPoint = zeros(delta-1, 2);
%         is_smoothing_valid = true;
%         
%         for j = 1:delta-1
%             alpha = j/delta;
%             smoothingPoint(j,:) = (1-alpha) * midPoints(idx_start,:) + alpha * midPoints(idx_end,:);
%             leftMargin = leftPoints(idx_start+j,:) - smoothingPoint(j,:);
%             rightMargin = rightPoints(idx_start+j,:) - smoothingPoint(j,:);
%             if dot(leftMargin, rightMargin) >= 0 || norm(leftMargin) < smoothing_margin || norm(rightMargin) < smoothing_margin
%                 % collision occured!
%                 is_smoothing_valid = false;
%             end
%         end
%         
%         if is_smoothing_valid
%             for j = 1:delta-1
%                 midPoints(idx_start+j,:) = smoothingPoint(j,:);
%             end
%         else
%             break;
%         end
%     end
%     i = i + 1;  
% end

%         smoothingPoint = zeros(delta-1, 2);
%         is_smoothing_valid = true;
%         
%         for j = 1:delta-1
%             alpha = j/delta;
%             smoothingPoint(j,:) = (1-alpha) * midPoints(idx_start,:) + alpha * midPoints(idx_end,:);
%             leftMargin = leftPoints(idx_start+j,:) - smoothingPoint(j,:);
%             rightMargin = rightPoints(idx_start+j,:) - smoothingPoint(j,:);
%             if dot(leftMargin, rightMargin) >= 0 || norm(leftMargin) < smoothing_margin || norm(rightMargin) < smoothing_margin
%                 % collision occured!
%                 is_smoothing_valid = false;
%             end
%         end
%         
%         if is_smoothing_valid
%             for j = 1:delta-1
%                 midPoints(idx_start+j,:) = smoothingPoint(j,:);
%             end
%         end
%     endmsgMap.Header.Stamp.Sec + msgMap.Header.Stamp.Nsec / 1000000000
% end

%% Time allocation
ts = zeros(size(midPoints, 1), 1);
ts(1) = 0;
for i_ts = 2:size(ts, 1)
    ts(i_ts) = ts(i_ts-1) + norm(midPoints(i_ts, :) - midPoints(i_ts-1, :)) / nominal_speed;
end

%% Generate Corridor
epsilon = 0.0001;
corridorPoints = [];
corridors = [];
count = 0;
for t_corr = current_time : corridor_dt : current_time + corridor_horizon
    count = count + 1;
    corridorPoint = findMidPoint(midPoints, ts, t_corr);
    width = findLaneWidth(laneGridWidth, ts, t_corr) / 2;
    
    corridorPoints = [corridorPoints; corridorPoint];
    
    if checkOccupancy(map, corridorPoint) > threshold
        error("corridor Point occluded!");
    end
    
    ld = corridorPoint + [-width, -width];
    ru = corridorPoint + [width, width];
    
    for x_corr = corridorPoint(1) - width : map_resolution : corridorPoint(1) + width
        for y_corr = corridorPoint(2) - width : map_resolution : corridorPoint(2) + width
            checkPoint = [x_corr, y_corr];
            if checkPoint(1) > ld(1) - epsilon && checkPoint(1) < ru(1) + epsilon && checkPoint(2) > ld(2) - epsilon && checkPoint(2) < ru(2) + epsilon
                if checkOccupancy(map, checkPoint) > threshold
                    delta = checkPoint - corridorPoint;
                    angle = atan2(delta(2), delta(1));
                    if angle > -pi/4-epsilon && angle < pi/4+epsilon
                        ru(1) = checkPoint(1) - map_resolution;
                    elseif angle > 3*pi/4-epsilon || angle < -3*pi/4+epsilon
                        ld(1) = checkPoint(1) + map_resolution; 
                    end
                    
                    if angle > pi/4-epsilon && angle < 3*pi/4+epsilon
                        ru(2) = checkPoint(2) - map_resolution;
                    elseif angle > -3*pi/4-epsilon && angle < -pi/4+epsilon
                        ld(2) = checkPoint(2) + map_resolution; 
                    end
                end
            end
        end
    end
    
    corridors = [corridors; [ld ru-ld]];
    
    delta = ru-ld;
    if delta(1) < 0 || delta(2) < 0
        disp("invalid square")
    end
    if checkOccupancy(map, ld) > threshold
        disp("invalid square")
    end
    if checkOccupancy(map, ru) > threshold
        disp("invalid square")
    end
    
    
end


%% Plot map and grid
hold on
show(map)
scatter(midPoints(:,1), midPoints(:,2), '.', 'b')
scatter(leftPoints(:,1), leftPoints(:,2), '.', 'r')
scatter(rightPoints(:,1), rightPoints(:,2), '.', 'g')
scatter(corridorPoints(:,1), corridorPoints(:,2),  'filled');
for i_corr = 1:size(corridors,1)
    rectangle('Position',corridors(i_corr,:));
end
hold off

% figure
% plot(midGridAngle)

pause(0.01)
end





















